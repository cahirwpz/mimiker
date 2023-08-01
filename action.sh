#!/bin/sh

case $1 in
  build)
    case $2 in
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
    case $2 in
      mips-*)
        ./run_tests.py --board malta --timeout=100 --times=50 ;;
      aarch64-*)
        ./run_tests.py --board rpi3 --timeout=60 --times=50 ;;
      riscv64-*)
        ./run_tests.py --board sifive_u --timeout=100 --times=50 ;;
      *)
        exit 1 ;;
    esac ;;
  *)
    exit 1 ;;
esac
