#!/usr/bin/python3

import argparse
import pexpect
import sys
import random

N_SIMPLE = 5
N_THOROUGH = 100
TIMEOUT = 10
RETRIES_MAX = 5
REPEAT = 5


# Tries to decode binary output as ASCII, as hard as it can.
def safe_decode(data):
    return data.decode('unicode_escape', errors='replace')


# Tries to start gdb in order to investigate kernel state on deadlock.
def gdb_inspect():
    gdb_command = 'mipsel-mimiker-elf-gdb'
    # Note: These options are different than .gdbinit.
    gdb_opts = ['-nx',
                'mimiker.elf',
                '-ex=target remote localhost:1234',
                '-ex=source debug/kdump.py']
    gdb = pexpect.spawn(gdb_command, gdb_opts, timeout=1)
    gdb.expect_exact('(gdb)', timeout=2)
    gdb.sendline('info registers')
    gdb.expect_exact('(gdb)')
    print(gdb.before.decode("ascii"))
    gdb.sendline('frame')
    gdb.expect_exact('(gdb)')
    print(gdb.before.decode("ascii"))
    gdb.sendline('backtrace')
    gdb.expect_exact('(gdb)')
    print(gdb.before.decode("ascii"))
    gdb.sendline('list')
    gdb.expect_exact('(gdb)')
    print(gdb.before.decode("ascii"))
    gdb.sendline('kdump threads')
    gdb.expect_exact('(gdb)')
    print(gdb.before.decode("ascii"))


def test_seed(seed, sim='qemu', repeat=1, retry=0):
    if retry == RETRIES_MAX:
        print("Maximum retries reached, still not output received. "
              "Test inconclusive.")
        sys.exit(1)

    print("Testing seed %d..." % seed)
    child = pexpect.spawn('./launch', ['-t', '-S', sim, 'test=all',
                                       'seed=%d' % seed, 'repeat=%d' % repeat])
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
        sys.exit(1)
    elif index == 2:
        print("EOF reached without success report. This may indicate "
              "a problem with the testing framework or QEMU. "
              "Retrying (%d)..." % (retry + 1))
        test_seed(seed, repeat, retry + 1)
    elif index == 3:
        print("Timeout reached.\n")
        message = safe_decode(child.buffer)
        print(message)
        if len(message) < 100:
            print("It looks like kernel did not even start within the time "
                  "limit. Retrying (%d)..." % (retry + 1))
            child.terminate(True)
            test_seed(seed, repeat, retry + 1)
        else:
            gdb_inspect()
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
                        help='Keep testing until some error'
                        ' is found. ')
    parser.add_argument('--ovpsim', action='store_true',
                        help='Use OVPSim instead of QEMU.')

    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(0)

    n = N_SIMPLE
    if args.thorough:
        n = N_THOROUGH

    # QEMU takes slightly less time to start, thus it is the default option for
    # running tests on multiple seeds.
    sim = 'qemu'
    if args.ovpsim:
        sim = 'ovpsim'

    # Run tests in alphabetic order
    test_seed(0, sim)
    # Run infinitely many tests, until some problem is found.
    if args.infinite:
        while True:
            seed = random.randint(0, 2**32)
            test_seed(seed, sim, REPEAT)
    # Run tests using n random seeds
    for i in range(0, n):
        seed = random.randint(0, 2**32)
        test_seed(seed, sim, REPEAT)

    print("Tests successful!")
    sys.exit(0)
