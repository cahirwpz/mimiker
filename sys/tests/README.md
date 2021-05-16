Test infrastructure
---

### Running tests

If you just want to run all available tests please use `./run_tests.py`. This is
the command that is used by continuous integration. Some useful arguments are
listed below:

* `--infinite` - keep testing until some error is found.
* `--non-interactive` - do not run interactive gdb session if tests fail.
* `--thorough` - generate much more test seeds. Testing will take much more time.

For greater control you can pass kernel arguments that control test runs. That
includes passing `seed` which is used to generate tests permutation, which
proved to be useful. Some useful kernel parameters for test run control are
listed below:

* `test=TEST` - Requests the kernel to run the specified test.
  `test=user_{name}` to run single user test, `test={name}` to run single kernel
  test.
  You can specify multiple tests by separating their names with commas (without spaces),
  e.g. `test=test1,test2,test3`
* `test=all` - Runs a number of tests one after another, and reports success
  only when all of them passed.
* `seed=UINT` - Sets the RNG seed for shuffling the list of test when using
  `test=all`.
* `repeat=UINT` - Specifies the number of (shuffled) repetitions of each test
  when using `test=all`.
  When running user-specified tests (i.e. when `test` is not equal to `all`),
  `repeat` specifies the number of times each test will be run. For example,
  `test=test1,test2 repeat=4` will run `test1` 4 times and then run `test2` 4 times.

### Programming tests

##### Kernel tests
Located in `$(TOPDIR)/tests`.
Test function signature looks like this: `{name}(void)` or sometimes
`{name}(unsigned int)` but needs to be coerced to `(int (*)(void))`.

Macros to register tests:
* `KTEST_ADD(name, func, flags)`
* `KTEST_ADD_RANDINT(name, func, flags, max)` - need to cast function pointer to
  `(int (*)(void))`

Where `name` is test name, `func` is pointer to test function,
flags as mentioned below, and `max` is maximum random argument fed to the test.

##### User tests
Located in `$(TOPDIR)/bin/utest`.
User-space test function signature looks like this: `int test_{name}(void)` and
should be defined in `bin/utest/utest.h`.
In order to make the test runnable one has to add one of these lines to
`test/utest.c` file:
* `UTEST_ADD_SIMPLE({name})` - test fails on assertion or non-zero return value.
* `UTEST_ADD_SIGNAL({name}, {SIGNUMBER})` - test passes when terminated with
  `{SIGNUMBER}`.
* `UTEST_ADD({name}, {exit status}, flags)` - test passes when exited with
  status `{exit status}`.

One also needs to add a line `CHECKRUN_TEST({name})` in `/bin/utest/main.c`.

##### Flags
* `KTEST_FLAG_NORETURN` - signifies that a test does not return.
* `KTEST_FLAG_DIRTY` - signifies that a test irreversibly breaks internal kernel
  state, and any further test done without restarting the kernel will be
  inconclusive.
* `KTEST_FLAG_USERMODE` - indicates that a test enters user mode.
* `KTEST_FLAG_BROKEN` - excludes the test from being run in auto mode. This flag
  is only useful for temporarily marking some tests while debugging the testing
  framework.
* `KTEST_FLAG_RANDINT` - marks that the test wishes to receive a random integer
  as an argument.
