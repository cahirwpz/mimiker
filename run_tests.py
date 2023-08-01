#!/usr/bin/env -S python3 -u

import argparse
import os
import random
import shutil
import signal
import subprocess
import sys


N_SIMPLE = 10
DEFAULT_TIMEOUT = 40
REPEAT = 5


def setup_terminal():
    cols, rows = shutil.get_terminal_size(fallback=(132, 43))

    os.environ['COLUMNS'] = str(cols)
    os.environ['LINES'] = str(rows)

    if sys.stdin.isatty():
        subprocess.run(['stty', 'cols', str(cols), 'rows', str(rows)])


def run_test(seed, board, timeout, suite=None):
    print("Testing seed %u..." % seed)

    try:
        launch = subprocess.Popen(
                ['./launch', '--board', board, '-t', '--timeout=%d' % timeout,
                 '%s=all' % suite, 'seed=%u' % seed, 'repeat=%d' % REPEAT])
        rc = launch.wait()
        if rc:
            print("Run `launch -d -b %s %s=all seed=%u repeat=%u` "
                  "to reproduce the failure." % (board, suite, seed, REPEAT))
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
    parser.add_argument('-b', '--board', default='rpi3',
                        choices=['malta', 'rpi3', 'sifive_u'],
                        help='Emulated board.')
    parser.add_argument('-s', '--suite', default='all',
                        choices=['all', 'user', 'kernel'],
                        help='Test suite to run.')
    parser.add_argument('-T', '--timeout', type=int, default=DEFAULT_TIMEOUT,
                        help='Test-run will fail after n seconds.')
    args = parser.parse_args()

    # Run tests using n random seeds
    for _ in range(0, args.times):
        rand = random.randint(0, 2**32)
        if args.suite == 'all':
            run_test(rand, args.board, args.timeout, 'ktest')
            run_test(rand, args.board, args.timeout, 'utest')
        elif args.suite == 'kernel':
            run_test(rand, args.board, args.timeout, 'ktest')
        elif args.suite == 'user':
            run_test(rand, args.board, args.timeout, 'utest')

    print("Tests successful!")
    sys.exit(0)
