FROM debian:stretch-backports

WORKDIR /root

COPY requirements.txt .

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
      git make cpio curl exuberant-ctags cscope rsync socat patch \
      gcc libc6-dev python3-pip clang-format-6.0 device-tree-compiler \
      libfdt1 libpython3.5 libsdl2-2.0-0 libglib2.0-0 libpixman-1-0
# rsync required by verify-format.sh
# gcc & libc6-dev required by newlib-3.0.0
# patch required by bin/lua
# socat required by launch
RUN pip3 install -r requirements.txt
RUN curl -O http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.3_amd64.deb && \
    dpkg -i mipsel-mimiker-elf_1.3_amd64.deb && rm -f mipsel-mimiker-elf_1.3_amd64.deb
RUN curl -O http://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_1.3_amd64.deb && \
    dpkg -i aarch64-mimiker-elf_1.3_amd64.deb && rm -f aarch64-mimiker-elf_1.3_amd64.deb
RUN curl -O http://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_2.12.0+mimiker2_amd64.deb && \
    dpkg -i qemu-mimiker_2.12.0+mimiker2_amd64.deb && rm -f qemu-mimiker_2.12.0+mimiker2_amd64.deb
