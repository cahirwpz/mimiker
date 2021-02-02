#!/usr/bin/env python3

import argparse
import pexpect
import random
import signal
import sys
from launcher import getvar, setboard, setup_terminal


N_SIMPLE = 10
TIMEOUT = 40
REPEAT = 5


# Tries to decode binary output as ASCII, as hard as it can.
def safe_decode(data):
    s = data.decode('unicode_escape', errors='replace')
    return s.replace('\r', '')


def test_seed(seed, repeat=1):
    print("Testing seed %u..." % seed)
    child = pexpect.spawn('./launch',
                          ['--board', getvar('board'),
                           '-t', 'test=all', 'klog-quiet=1',
                           'seed=%u' % seed, 'repeat=%d' % repeat])
    index = child.expect_exact([pexpect.EOF, pexpect.TIMEOUT], timeout=TIMEOUT)
    if index == 0:
        child.close()
        if child.exitstatus == 0:
            return
        child.terminate(True)
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
    elif index == 1:
        print("Timeout reached!\n")
        message = safe_decode(child.buffer)
        child.terminate(True)
        print(message)
        print("No test result reported within timeout. Unable to verify "
              "test success. Seed was: %u, repeat: %d" % (seed, repeat))
        sys.exit(1)


def sigterm_handler(_signo, _stack_frame):
    sys.exit(1)


if __name__ == '__main__':
    signal.signal(signal.SIGTERM, sigterm_handler)
    signal.signal(signal.SIGINT, sigterm_handler)
    signal.signal(signal.SIGHUP, sigterm_handler)

    setup_terminal()

    parser = argparse.ArgumentParser(
        description='Automatically performs kernel tests.')
    parser.add_argument('--times', type=int, default=N_SIMPLE,
                        help='Run tests given number of times.')
    parser.add_argument('--board', default='malta', choices=['malta', 'rpi3'],
                        help='Emulated board.')
    args = parser.parse_args()

    setboard(args.board)

    # Run tests using n random seeds
    for i in range(0, args.times):
        seed = random.randint(0, 2**32)
        test_seed(seed, REPEAT)

    print("Tests successful!")
    sys.exit(0)
