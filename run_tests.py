#!/usr/bin/env python3

import argparse
import pexpect
import sys
import random
import os
from launcher import gdb_port, TARGET


N_SIMPLE = 5
N_THOROUGH = 100
TIMEOUT = 20
RETRIES_MAX = 5
REPEAT = 5


# Tries to decode binary output as ASCII, as hard as it can.
def safe_decode(data):
    return data.decode('unicode_escape', errors='replace').replace('\r', '')


def send_command(gdb, cmd):
    gdb.expect_exact(['(gdb)'], timeout=10)
    gdb.sendline(cmd)
    print(safe_decode(gdb.before), end='', flush=True)
    print(safe_decode(gdb.after), end='', flush=True)


# Tries to start gdb in order to investigate kernel state on deadlock or crash.
def gdb_inspect(interactive):
    gdb_cmd = TARGET + '-mimiker-elf-gdb'
    if interactive:
        gdb_opts = ['-iex=set auto-load safe-path {}/'.format(os.getcwd()),
                    '-ex=target remote localhost:%d' % gdb_port(),
                    '--silent', 'mimiker.elf']
    else:
        # Note: These options are different than .gdbinit.
        gdb_opts = ['-ex=target remote localhost:%d' % gdb_port(),
                    '-ex=python import os, sys',
                    '-ex=python sys.path.append(os.getcwd())',
                    '-ex=python import debug',
                    '-ex=set pagination off',
                    '--nh', '--nx', '--silent', 'mimiker.elf']
    gdb = pexpect.spawn(gdb_cmd, gdb_opts, timeout=3)
    if interactive:
        send_command(gdb, 'backtrace')
        send_command(gdb, 'kthread')
        gdb.interact()
    else:
        send_command(gdb, 'info registers')
        send_command(gdb, 'backtrace')
        # following commands may fail
        send_command(gdb, 'kthread')
        send_command(gdb, 'klog')
        # we need dummy command - otherwise previous one won't appear
        send_command(gdb, 'quit')


def test_seed(seed, interactive=True, repeat=1, retry=0):
    if retry == RETRIES_MAX:
        print("Maximum retries reached, still not output received. "
              "Test inconclusive.")
        sys.exit(1)

    print("Testing seed %u..." % seed)
    child = pexpect.spawn('./launch',
                          ['-t', 'test=all', 'klog-quiet=1', 'seed=%u' % seed,
                           'repeat=%d' % repeat])
    index = child.expect_exact(
        ['[TEST PASSED]', '[TEST FAILED]', pexpect.EOF, pexpect.TIMEOUT],
        timeout=TIMEOUT)
    if index == 0:
        child.terminate(True)
        return
    elif index == 1:
        print("Test failure reported!\n")
        message = safe_decode(child.before)
        message += safe_decode(child.buffer)
        try:
            while len(message) < 20000:
                message += safe_decode(child.read_nonblocking(timeout=1))
        except pexpect.exceptions.TIMEOUT:
            pass
        print(message)
        gdb_inspect(interactive)
        sys.exit(1)
    elif index == 2:
        message = safe_decode(child.before)
        message += safe_decode(child.buffer)
        print(message)
        print("EOF reached without success report. This may indicate "
              "a problem with the testing framework or QEMU. "
              "Retrying (%d)..." % (retry + 1))
        test_seed(seed, interactive, repeat, retry + 1)
    elif index == 3:
        print("Timeout reached.\n")
        message = safe_decode(child.buffer)
        print(message)
        if len(message) < 100:
            print("It looks like kernel did not even start within the time "
                  "limit. Retrying (%d)..." % (retry + 1))
            child.terminate(True)
            test_seed(seed, interactive, repeat, retry + 1)
        else:
            gdb_inspect(interactive)
            print("No test result reported within timeout. Unable to verify "
                  "test success. Seed was: %u, repeat: %d" % (seed, repeat))
            sys.exit(1)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Automatically performs kernel tests.')
    parser.add_argument('--thorough', action='store_true',
                        help='Generate much more test seeds.'
                        'Testing will take much more time.')
    parser.add_argument('--infinite', action='store_true',
                        help='Keep testing until some error is found.')
    parser.add_argument('--non-interactive', action='store_true',
                        help='Do not run gdb session if tests fail.')

    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(0)

    n = N_SIMPLE
    if args.thorough:
        n = N_THOROUGH

    interactive = not args.non_interactive

    # Run tests in alphabetic order
    test_seed(0, interactive)
    # Run infinitely many tests, until some problem is found.
    if args.infinite:
        while True:
            seed = random.randint(0, 2**32)
            test_seed(seed, interactive, REPEAT)
    # Run tests using n random seeds
    for i in range(0, n):
        seed = random.randint(0, 2**32)
        test_seed(seed, interactive, REPEAT)

    print("Tests successful!")
    sys.exit(0)
