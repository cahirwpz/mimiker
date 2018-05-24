Test infrastructure
---

##### User tests
Located in `/user/utest`.
User-space test function signature looks like this: `int test_{name}(void)` 
and should be defined in `user/utest/utest.h`.
In order to make the test runnable one has to add one of these lines to `test/utest.c` file:
* `UTEST_ADD_SIMPLE({name})` - test fails on assertion or non-zero return value.
* `UTEST_ADD_SIGNAL({name}, {SIGNUMBER})` - test passes when terminated with `{SIGNUMBER}`.
* `UTEST_ADD({name}, {exit status}, flags)` - test passes when exited with status `{exit status}`.

One also needs to add a line `CHECKRUN_TEST({name})` in `/user/utest/main.c`.

##### Kernel tests 
Located in `/tests`. 
Test function signature looks like this:
`{name}(void)` or sometimes `{name}(unsigned int)` but needs to be casted to 
`(int (*)(void))`.

Macros to register tests:
* `KTEST_ADD(name, func, flags)`
* `KTEST_ADD_RANDINT(name, func, flags, max)` - need to cast function pointer to
`(int (*)(void))`

Where `name` is test name, `func` is pointer to test function, 
flags as mentioned below, and `max` is maximum random argument fed to the test.

##### Flags
* `KTEST_FLAG_NORETURN` - signifies that a test does not return.
* `KTEST_FLAG_DIRTY` - signifies that a test irreversibly breaks internal kernel state, and any
   further test done without restarting the kernel will be inconclusive.
* `KTEST_FLAG_USERMODE` - indicates that a test enters usermode.
* `KTEST_FLAG_BROKEN` - excludes the test from being run in auto mode. This flag is only useful for
   temporarily marking some tests while debugging the testing framework.
* `KTEST_FLAG_RANDINT` - marks that the test wishes to receive a random integer as an argument.

###### `./launch` test-related arguments:
* `test=TEST` - Requests the kernel to run the specified test.
`test=user_{name}` to run single user test, `test={name}` to run single kernel test.
* `test=all` - Runs a number of tests one after another, and reports success
  only when all of them passed.
* `seed=UINT` - Sets the RNG seed for shuffling the list of test when using
  `test=all`.
* `repeat=UINT` - Specifies the number of (shuffled) repetitions of each test
  when using `test=all`.

###### `./run_tests.py` useful arguments:
* `-h` - prints script usage.
* `--infinite` - keep testing until some error is found. 
* `--non-interactive` - do not run interactive gdb session if tests fail.
* `--thorough` - generate much more test seeds. Testing will take much more time.
