#!/bin/bash

# This script is created based on Dockerfile.

set -x

apt-get -q update && apt-get upgrade -y
apt-get install -y --no-install-recommends -t bullseye-backports \
      git make ccache cpio curl gnupg universal-ctags cscope socat patch \
      gperf quilt bmake byacc python3-pip device-tree-compiler tmux \
      libmpfr6 libfdt1 libpython3.9 libsdl2-2.0-0 libglib2.0-0 libpixman-1-0
apt-key adv --fetch-keys https://apt.llvm.org/llvm-snapshot.gpg.key
echo "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-14 main" > \
      /etc/apt/sources.list.d/llvm-14.list && apt-get update
apt-get install -y --no-install-recommends -t bullseye-backports \
      clang-14 clang-format-14 llvm-14 lld-14

# patch & quilt required by lua and programs in contrib/
# gperf required by libterminfo
# socat & tmux required by launch

pip3 install setuptools wheel && pip3 install -r requirements.txt
curl -O https://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_latest_amd64.deb && \
    dpkg -i mipsel-mimiker-elf_latest_amd64.deb && rm -f mipsel-mimiker-elf_latest_amd64.deb
curl -O https://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_latest_amd64.deb && \
    dpkg -i aarch64-mimiker-elf_latest_amd64.deb && rm -f aarch64-mimiker-elf_latest_amd64.deb
curl -O https://mimiker.ii.uni.wroc.pl/download/riscv32-mimiker-elf_latest_amd64.deb && \
    dpkg -i riscv32-mimiker-elf_latest_amd64.deb && rm -f riscv32-mimiker-elf_latest_amd64.deb
curl -O https://mimiker.ii.uni.wroc.pl/download/riscv64-mimiker-elf_latest_amd64.deb && \
    dpkg -i riscv64-mimiker-elf_latest_amd64.deb && rm -f riscv64-mimiker-elf_latest_amd64.deb
curl -O https://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_latest_amd64.deb && \
    dpkg -i qemu-mimiker_latest_amd64.deb && rm -f qemu-mimiker_latest_amd64.deb
