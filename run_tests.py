#!/usr/bin/python3

import argparse
import pexpect
import sys
import random
import os

N_SIMPLE = 5
N_THOROUGH = 100
TIMEOUT = 20
RETRIES_MAX = 5
REPEAT = 5

GDB_PORT_BASE = 9100


# Tries to decode binary output as ASCII, as hard as it can.
def safe_decode(data):
    return data.decode('unicode_escape', errors='replace').replace('\r', '')


def send_command(gdb, cmd):
    print('\n>>>', end='', flush=True)
    gdb.setecho(False)
    gdb.sendline(cmd)
    while True:
        index = gdb.expect_exact(['(gdb)', '---Type <return> to continue, '
                                  'or q <return> to quit---'], timeout=2)
        print(safe_decode(gdb.before).lstrip('\n'), end='', flush=True)
        if index == 0:
            break
        gdb.send('\n')
    gdb.setecho(True)


# Tries to start gdb in order to investigate kernel state on deadlock or crash.
def gdb_inspect(interactive, commands):
    gdb_port = GDB_PORT_BASE + os.getuid()
    gdb_command = 'mipsel-mimiker-elf-gdb'
    # Note: These options are different than .gdbinit.
    gdb_opts = ['-ex=target remote localhost:%d' % gdb_port,
                '-ex=python import os, sys',
                '-ex=python sys.path.append(os.getcwd())',
                '-ex=python import debug',
                '--nh', '--nx', 'mimiker.elf']
    gdb = pexpect.spawn(gdb_command, gdb_opts, timeout=3)
    gdb.expect_exact('(gdb)', timeout=5)
    for cmd in commands:
        send_command(gdb, cmd)

    if interactive:
        gdb.interact()


def test_seed(seed, gdb_cmds, interactive=True, repeat=1, retry=0):
    if retry == RETRIES_MAX:
        print("Maximum retries reached, still not output received. "
              "Test inconclusive.")
        sys.exit(1)

    print("Testing seed %d..." % seed)
    child = pexpect.spawn('./launch',
                          ['-t', 'test=all', 'klog-quiet=1', 'seed=%d' % seed,
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
        gdb_inspect(interactive, gdb_cmds)
        sys.exit(1)
    elif index == 2:
        message = safe_decode(child.before)
        message += safe_decode(child.buffer)
        print(message)
        print("EOF reached without success report. This may indicate "
              "a problem with the testing framework or QEMU. "
              "Retrying (%d)..." % (retry + 1))
        test_seed(seed, gdb_cmds, interactive, repeat, retry + 1)
    elif index == 3:
        print("Timeout reached.\n")
        message = safe_decode(child.buffer)
        print(message)
        if len(message) < 100:
            print("It looks like kernel did not even start within the time "
                  "limit. Retrying (%d)..." % (retry + 1))
            child.terminate(True)
            test_seed(seed, gdb_cmds, interactive, repeat, retry + 1)
        else:
            gdb_inspect(interactive, gdb_cmds)
            print("No test result reported within timeout. Unable to verify "
                  "test success. Seed was: %d, repeat: %d" % (seed, repeat))
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
    parser.add_argument('--commands', nargs='*', metavar='CMD',
                        help='Commands to automatically run in gdb session when a test fails',
                        default=['klog', 'info registers', 'backtrace', 'list', 'kthread'])

    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(0)

    n = N_SIMPLE
    if args.thorough:
        n = N_THOROUGH

    interactive = not args.non_interactive

    # Run tests in alphabetic order
    test_seed(0, args.commands, interactive)
    # Run infinitely many tests, until some problem is found.
    if args.infinite:
        while True:
            seed = random.randint(0, 2**32)
            test_seed(seed, args.commands, interactive, REPEAT)
    # Run tests using n random seeds
    for i in range(0, n):
        seed = random.randint(0, 2**32)
        test_seed(seed, args.commands, interactive, REPEAT)

    print("Tests successful!")
    sys.exit(0)
