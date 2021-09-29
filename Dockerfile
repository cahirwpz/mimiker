# First construct an image where the toolchain gets built

FROM debian:bullseye-backports AS builder

WORKDIR /root
RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
      automake bison ca-certificates curl flex gcc g++ gettext lhasa \
      libtool make patch pkg-config python3 python3-setuptools quilt \
      texinfo xz-utils zip libpython3-dev debhelper fakeroot ninja-build \
      libglib2.0-dev libfdt-dev libpixman-1-dev \
    && rm -rf /var/lib/apt/lists/*
COPY . /root/mimiker
RUN cd /root/mimiker/toolchain/gnu \
    && make -j $(nproc)  \
    && cd /root/mimiker/toolchain/qemu-mimiker \
    && make -j $(nproc) \
    && mv /root/mimiker/toolchain/gnu/mipsel-mimiker-elf_1.5.3_amd64.deb /root/mipsel-mimiker-elf_1.5.3_amd64.deb \
    && mv /root/mimiker/toolchain/gnu/aarch64-mimiker-elf_1.5.3_amd64.deb /root/aarch64-mimiker-elf_1.5.3_amd64.deb \
    && mv /root/mimiker/toolchain/qemu-mimiker_6.1.0+mimiker1_amd64.deb /root/qemu-mimiker_6.1.0+mimiker1_amd64.deb \
    && rm -rf /root/mimiker

# Now construct an image for end users and Mimiker continuous integration
FROM debian:bullseye-backports

WORKDIR /root
RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
      git make cpio curl universal-ctags cscope rsync socat patch gperf quilt \
      bmake byacc python3-pip clang clang-format device-tree-compiler \
      libfdt1 libpython3.9 libsdl2-2.0-0 libglib2.0-0 libpixman-1-0 \
      python3-libtmux tmux \
    && rm -rf /var/lib/apt/lists/*
# rsync required by verify-format.sh
# patch & quilt required by lua and programs in contrib/
# gperf required by libterminfo
# socat required by launch
COPY requirements.txt .
RUN pip3 install setuptools wheel && pip3 install -r requirements.txt
COPY --from=builder /root/mipsel-mimiker-elf_1.5.3_amd64.deb .
COPY --from=builder /root/aarch64-mimiker-elf_1.5.3_amd64.deb .
COPY --from=builder /root/qemu-mimiker_6.1.0+mimiker1_amd64.deb .
RUN dpkg -i --force-all ./mipsel-mimiker-elf_1.5.3_amd64.deb \
                        ./aarch64-mimiker-elf_1.5.3_amd64.deb \
                        ./qemu-mimiker_6.1.0+mimiker1_amd64.deb \
    && apt-get update \
    && apt-get install -y -f \
    && rm -rf /var/lib/apt/lists/* \
    && rm ./mipsel-mimiker-elf_1.5.3_amd64.deb \
          ./aarch64-mimiker-elf_1.5.3_amd64.deb \
          ./qemu-mimiker_6.1.0+mimiker1_amd64.deb
