FROM debian:bullseye-backports

WORKDIR /root

COPY packages.txt .
RUN apt-get -q update && apt-get upgrade -y && \
  apt-get install -y --no-install-recommends -t bullseye-backports $(cat packages.txt) && \
  apt-key adv --fetch-keys https://apt.llvm.org/llvm-snapshot.gpg.key && \
  echo "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-14 main" > \
        /etc/apt/sources.list.d/llvm-14.list && apt-get update && \
  apt-get install -y --no-install-recommends -t bullseye-backports \
        clang-14 clang-format-14 llvm-14 lld-14

COPY requirements.txt .
RUN pip3 install setuptools wheel && pip3 install -r requirements.txt && \
  curl -O https://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_latest_amd64.deb && \
  curl -O https://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_latest_amd64.deb && \
  curl -O https://mimiker.ii.uni.wroc.pl/download/riscv32-mimiker-elf_latest_amd64.deb && \
  curl -O https://mimiker.ii.uni.wroc.pl/download/riscv64-mimiker-elf_latest_amd64.deb && \
  curl -O https://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_latest_amd64.deb && \
  apt-get install -y ./*.deb && rm -f *.deb
