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


def run_test(seed, board, timeout, parallel, suite=None):
    print('Testing seed %u...' % seed)

    args = ['%s=all' % suite, 'seed=%u' % seed, 'repeat=%d' % REPEAT,
            'parallel=%d' % parallel]

    try:
        launch = subprocess.Popen(
                ['./launch', '-b', board, '-t', '-T', str(timeout)] + args)
        rc = launch.wait()
        if rc:
            print('Run `launch -d -b %s %s` to reproduce the failure.' %
                  (board, ' '.join(args)))
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
    parser.add_argument('-p', '--parallel', type=int, default=1,
                        help='Run at most N tests in parallel.')
    parser.add_argument('-s', '--suite', default='all',
                        choices=['all', 'user', 'kernel'],
                        help='Test suite to run.')
    parser.add_argument('-T', '--timeout', type=int, default=DEFAULT_TIMEOUT,
                        help='Test-run will fail after n seconds.')
    args = parser.parse_args()

    # Run tests using n random seeds
    for _ in range(0, args.times):
        rand = random.randint(0, 2**32)
        if args.suite in ['all', 'kernel']:
            run_test(rand, args.board, args.timeout, args.parallel, 'ktest')
        if args.suite in ['all', 'user']:
            run_test(rand, args.board, args.timeout, args.parallel, 'utest')

    print('Tests successful!')
    sys.exit(0)
