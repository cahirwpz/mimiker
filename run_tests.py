#!/usr/bin/env python3

import argparse
import random
import signal
import subprocess
import sys
from launcher import getvar, setboard, setup_terminal


N_SIMPLE = 10
TIMEOUT = 40
REPEAT = 5


def run_test(seed):
    print("Testing seed %u..." % seed)

    try:
        launch = subprocess.Popen(
                ['./launch', '--board', getvar('board'),
                 '-t', 'test=all', 'seed=%u' % seed, 'repeat=%d' % REPEAT],
                stdin=subprocess.PIPE)

        # Wait for launch to finish or timeout.
        try:
            rc = launch.wait(timeout=TIMEOUT)
            if rc:
                print("Test failure reported!")
                sys.exit(rc)
            return
        except subprocess.TimeoutExpired:
            print("Timeout reached!")

        # Since launch has timeouted interrupt it and gather post-mortem.
        launch.send_signal(signal.SIGINT)
        try:
            launch.communicate(b"post-mortem\n", timeout=15)
            launch.communicate(b"kill\n")
            launch.communicate(b"quit\n")
        except (subprocess.TimeoutExpired, ValueError):
            pass
        launch.terminate()

        print("No test result reported within timeout. Unable to verify "
              "test success. Seed was: %u, repeat: %d" % (seed, REPEAT))
        sys.exit(1)

    except KeyboardInterrupt:
        launch.terminate()
        sys.exit(1)


def sigterm_handler(_signo, _stack_frame):
    sys.exit(1)


if __name__ == '__main__':
    signal.signal(signal.SIGTERM, sigterm_handler)
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
    for _ in range(0, args.times):
        run_test(random.randint(0, 2**32))

    print("Tests successful!")
    sys.exit(0)
