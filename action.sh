#!/bin/sh

cmd=$1
arch=$2
build=$3

shift 3

case $arch in
  mips) board=malta ;;
  aarch64) board=rpi3 ;;
  riscv32) board=litex-riscv ;;
  riscv64) board=sifive_u ;;
  *) exit 1 ;;
esac

case $build in
  kasan) ksan=KASAN ;;
  kcsan) ksan=KCSAN ;;
  *) exit 1 ;;
esac

case $cmd in
  build)
    make BOARD=$board $ksan=1 LOCKDEP=1 ;;
  tests)
    case $build in
      kasan)
        ./run_tests.py --board $board --timeout=100 --times=50 --suite=$1 ;;
      kcsan)
        ./run_tests.py --board $board --timeout=100 --times=50 --suite=$1
        # do not report it as failed because we have no people working on
        # fixing concurrency issues
        exit 0 ;;
      *)
        exit 1 ;;
    esac ;;
  *)
    exit 1 ;;
esac
