FROM debian:bullseye-backports

WORKDIR /root

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends -t bullseye-backports \
      git make cpio curl universal-ctags cscope socat patch gperf quilt \
      bmake byacc python3-pip clang clang-format llvm lld \
      device-tree-compiler tmux libmpfr6 libfdt1 libpython3.9 \
      libsdl2-2.0-0 libglib2.0-0 libpixman-1-0
# patch & quilt required by lua and programs in contrib/
# gperf required by libterminfo
# socat & tmux required by launch
COPY requirements.txt .
RUN pip3 install setuptools wheel && pip3 install -r requirements.txt
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_latest_amd64.deb && \
    dpkg -i mipsel-mimiker-elf_latest_amd64.deb && rm -f mipsel-mimiker-elf_latest_amd64.deb
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_latest_amd64.deb && \
    dpkg -i aarch64-mimiker-elf_latest_amd64.deb && rm -f aarch64-mimiker-elf_latest_amd64.deb
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/riscv32-mimiker-elf_latest_amd64.deb && \
    dpkg -i riscv32-mimiker-elf_latest_amd64.deb && rm -f riscv32-mimiker-elf_latest_amd64.deb
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/riscv64-mimiker-elf_latest_amd64.deb && \
    dpkg -i riscv64-mimiker-elf_latest_amd64.deb && rm -f riscv64-mimiker-elf_latest_amd64.deb
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_latest_amd64.deb && \
    dpkg -i qemu-mimiker_latest_amd64.deb && rm -f qemu-mimiker_latest_amd64.deb
