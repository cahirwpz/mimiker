# First construct an image where the toolchain gets built

FROM debian:bullseye-backports AS mimiker-toolchain

WORKDIR /root
# Install build dependencies for toolchain
RUN apt-get update -q \
    && apt-get upgrade -q -y \
    && apt-get install -q -y --no-install-recommends \
    automake bison ca-certificates curl flex gcc g++ gettext lhasa \
    libtool make patch pkg-config python3 python3-setuptools quilt \
    texinfo xz-utils zip libpython3-dev debhelper fakeroot ninja-build \
    libglib2.0-dev libfdt-dev libpixman-1-dev \
    && rm -rf /var/lib/apt/lists/*
COPY ./toolchain /root/toolchain
# Build toolchain & save .deb packages into /root/deb
RUN cd /root/toolchain/gnu \
    && make -j $(nproc)  \
    && cd /root/toolchain/qemu-mimiker \
    && make -j $(nproc) \
    && mkdir /root/deb \
    && find /root/toolchain -iname '*.deb' -exec mv -t /root/deb {} + \
    && rm -rf /root/toolchain

# Now construct an image for end users and Mimiker continuous integration
FROM debian:bullseye-backports AS mimiker-ci

WORKDIR /root
COPY requirements.txt .
COPY --from=mimiker-toolchain /root/deb /root/deb
RUN apt-get update -q \
    && apt-get upgrade -q -y \
    &&  apt-get install -q -y --no-install-recommends \
      git make cpio curl universal-ctags cscope socat patch gperf quilt \
      bmake byacc python3-pip clang clang-format device-tree-compiler tmux \
      libfdt1 libpython3.9 libsdl2-2.0-0 libglib2.0-0 libpixman-1-0 \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install setuptools wheel \
    && pip3 install -r requirements.txt \
    && dpkg -i --force-all /root/deb/*.deb \
    && apt-get update -q \
    && apt-get install -q -y -f \
    && rm -rf /var/lib/apt/lists/* /root/deb /root/requirements.txt
# patch & quilt required by programs in contrib/
# gperf required by libterminfo
# socat & tmux required by launch
