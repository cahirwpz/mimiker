FROM debian:bullseye-backports

WORKDIR /root

RUN apt-get -q update && apt-get upgrade -y

COPY packages-ci.txt .
RUN apt-get install -y --no-install-recommends -t bullseye-backports $(cat packages-ci.txt)
RUN apt-key adv --fetch-keys https://apt.llvm.org/llvm-snapshot.gpg.key
RUN echo "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-14 main" > \
      /etc/apt/sources.list.d/llvm-14.list && apt-get update
RUN apt-get install -y --no-install-recommends -t bullseye-backports \
      clang-14 clang-format-14 llvm-14 lld-14

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
