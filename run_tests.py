#!/usr/bin/python3
import argparse
import pexpect
import sys
import random

N_SIMPLE = 5
N_THOROUGH = 200

parser = argparse.ArgumentParser(description='Automatically performs kernel tests.')
parser.add_argument('--thorough', action='store_true', help='Generate much more test seeds. Testing will take much more time.')

try:
    args = parser.parse_args()
except SystemExit:
    sys.exit(0)

n = N_SIMPLE
if args.thorough:
    n = N_THOROUGH

def test_seed(seed):
    print("Testing seed %d..." % seed)
    # QEMU takes much much less time to start, so for testing multiple seeds it
    # is more convenient to use it instead of OVPsim.
    child = pexpect.spawn(
        './launch', ['-t', '-S', 'qemu', 'test=all', 'seed=%d' % seed])
    index = child.expect_exact(
        ['[TEST PASSED]', '[TEST FAILED]', pexpect.EOF, pexpect.TIMEOUT], timeout=15)
    if index == 0:
        return
    elif index == 1:
        print("Test failure reported!")
        message = child.buffer.decode("ascii")
        try:
            while len(message) < 20000:
                message += child.read_nonblocking(timeout=1).decode("ascii") 
        except pexpect.exceptions.TIMEOUT:
            pass
        print(message)
        sys.exit(1)
    elif index == 2:
        print("EOF reached without success report. This may indicate a problem with the testing framework.")
        sys.exit(1)
    elif index == 3:
        print("No test result reported within timeout. Unable to verify test success.")
        sys.exit(1)

for i in range(0, n):
    seed = random.randint(0, 2**32)
    test_seed(seed)
    
print("Tests successful!")
sys.exit(0)
