FROM debian:bullseye-backports

WORKDIR /root

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
      git make cpio curl universal-ctags cscope socat patch gperf quilt \
      bmake byacc python3-pip clang clang-format device-tree-compiler tmux \
      libfdt1 libpython3.9 libsdl2-2.0-0 libglib2.0-0 libpixman-1-0
# patch & quilt required by lua and programs in contrib/
# gperf required by libterminfo
# socat & tmux required by launch
COPY requirements.txt .
RUN pip3 install setuptools wheel && pip3 install -r requirements.txt
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.5.3_amd64.deb && \
    dpkg -i mipsel-mimiker-elf_1.5.3_amd64.deb && rm -f mipsel-mimiker-elf_1.5.3_amd64.deb
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_1.5.3_amd64.deb && \
    dpkg -i aarch64-mimiker-elf_1.5.3_amd64.deb && rm -f aarch64-mimiker-elf_1.5.3_amd64.deb
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_6.1.0+mimiker1_amd64.deb && \
    dpkg -i qemu-mimiker_6.1.0+mimiker1_amd64.deb && rm -f qemu-mimiker_6.1.0+mimiker1_amd64.deb
