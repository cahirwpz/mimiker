# First construct an image where the toolchain gets built

FROM debian:bullseye-backports AS mimiker-toolchain

WORKDIR /root
RUN apt-get -q update \
    && apt-get upgrade -y
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
    && mv /root/mimiker/toolchain/gnu/mipsel-mimiker-elf_*_amd64.deb /root \
    && mv /root/mimiker/toolchain/gnu/aarch64-mimiker-elf_*_amd64.deb /root \
    && mv /root/mimiker/toolchain/qemu-mimiker_*_amd64.deb /root \
    && rm -rf /root/mimiker

# Now construct an image for end users and Mimiker continuous integration
FROM debian:bullseye-backports AS mimiker-ci

WORKDIR /root
RUN apt-get -q update \
    && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
      git make cpio curl universal-ctags cscope socat patch gperf quilt \
      bmake byacc python3-pip clang clang-format device-tree-compiler tmux \
      libfdt1 libpython3.9 libsdl2-2.0-0 libglib2.0-0 libpixman-1-0 \
    && rm -rf /var/lib/apt/lists/*
# patch & quilt required by programs in contrib/
# gperf required by libterminfo
# socat & tmux required by launch
COPY requirements.txt .
RUN pip3 install setuptools wheel && pip3 install -r requirements.txt
COPY --from=mimiker-toolchain /root/mipsel-mimiker-elf_*_amd64.deb .
COPY --from=mimiker-toolchain /root/aarch64-mimiker-elf_*_amd64.deb .
COPY --from=mimiker-toolchain /root/qemu-mimiker_*_amd64.deb .
RUN dpkg -i --force-all ./mipsel-mimiker-elf_*_amd64.deb \
                        ./aarch64-mimiker-elf_*_amd64.deb \
                        ./qemu-mimiker_*_amd64.deb \
    && apt-get update \
    && apt-get install -y -f \
    && rm -rf /var/lib/apt/lists/* \
    && rm ./mipsel-mimiker-elf_*_amd64.deb \
          ./aarch64-mimiker-elf_*_amd64.deb \
          ./qemu-mimiker_*_amd64.deb
