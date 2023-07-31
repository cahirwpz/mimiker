#!/bin/sh

case $1 in
  "mips-kasan")
    make BOARD=malta KASAN=1 LOCKDEP=1 ;;
  "mips-kcsan")
    make BOARD=malta KCSAN=1 LOCKDEP=1 ;;
  "aarch64-kasan")
    make BOARD=rpi3 KASAN=1 LOCKDEP=1 ;;
  "aarch64-kcsan")
    make BOARD=rpi3 KCSAN=1 LOCKDEP=1 ;;
  "riscv32-kasan")
    make BOARD=litex-riscv KASAN=1 LOCKDEP=1 ;;
  "riscv64-kasan")
    make BOARD=sifive_u KASAN=1 LOCKDEP=1 ;;
  "riscv64-kcsan")
    make BOARD=sifive_u KCSAN=1 LOCKDEP=1 ;;
  *)
    exit 1 ;;
esac
