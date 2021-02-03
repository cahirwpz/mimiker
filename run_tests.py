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
                ['./launch', '--board', getvar('board'), '--test-run',
                 '--timeout=%d' % TIMEOUT, 'test=all', 'seed=%u' % seed,
                 'repeat=%d' % REPEAT])
        rc = launch.wait()
        if rc:
            print("Run `launch -d test=all seed=%u repeat=%u` to reproduce "
                  "the failure." % (seed, REPEAT))
            sys.exit(rc)
    except KeyboardInterrupt:
        launch.send_signal(signal.SIGINT)
        sys.exit(launch.wait())


if __name__ == '__main__':
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
