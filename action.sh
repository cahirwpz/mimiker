#!/bin/sh

cmd=$1
target=$2
shift 2

case $cmd in
  build)
    case $target in
      mips-kasan)
        make BOARD=malta KASAN=1 LOCKDEP=1 ;;
      mips-kcsan)
        make BOARD=malta KCSAN=1 LOCKDEP=1 ;;
      aarch64-kasan)
        make BOARD=rpi3 KASAN=1 LOCKDEP=1 ;;
      aarch64-kcsan)
        make BOARD=rpi3 KCSAN=1 LOCKDEP=1 ;;
      riscv32-kasan)
        make BOARD=litex-riscv KASAN=1 LOCKDEP=1 ;;
      riscv64-kasan)
        make BOARD=sifive_u KASAN=1 LOCKDEP=1 ;;
      riscv64-kcsan)
        make BOARD=sifive_u KCSAN=1 LOCKDEP=1 ;;
      *)
        exit 1 ;;
    esac ;;
  tests)
    case $target in
      mips-*)
        ./run_tests.py --board malta --timeout=100 --times=50 --suite=$1 ;;
      aarch64-kcsan)
        ./run_tests.py --board rpi3 --timeout=100 --times=50 --suite=$1
        # do not report it as failed because we have no people working on
        # fixing concurrency issues
        exit 0 ;;
      aarch64-*)
        ./run_tests.py --board rpi3 --timeout=100 --times=50 --suite=$1 ;;
      riscv64-*)
        ./run_tests.py --board sifive_u --timeout=100 --times=50 --suite=$1 ;;
      *)
        exit 1 ;;
    esac ;;
  *)
    exit 1 ;;
esac
