FROM debian:buster-backports

WORKDIR /root

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
      git make cpio curl exuberant-ctags cscope rsync socat patch gperf quilt \
      bmake byacc python3-pip clang-8 clang-format-8 device-tree-compiler \
      libfdt1 libpython3.7 libsdl2-2.0-0 libglib2.0-0 libpixman-1-0
# rsync required by verify-format.sh
# patch & quilt required by lua and programs in contrib/
# gperf required by libterminfo
# socat required by launch
COPY requirements.txt .
RUN ln -s /usr/bin/clang-8 /usr/local/bin/clang
RUN ln -s /usr/bin/clang-format-8 /usr/local/bin/clang-format
RUN pip3 install setuptools wheel && pip3 install -r requirements.txt
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.5.1_amd64.deb && \
    dpkg -i mipsel-mimiker-elf_1.5.1_amd64.deb && rm -f mipsel-mimiker-elf_1.5.1_amd64.deb
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_1.5.1_amd64.deb && \
    dpkg -i aarch64-mimiker-elf_1.5.1_amd64.deb && rm -f aarch64-mimiker-elf_1.5.1_amd64.deb
RUN curl -O https://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_4.2.1+mimiker3_amd64.deb && \
    dpkg -i qemu-mimiker_4.2.1+mimiker3_amd64.deb && rm -f qemu-mimiker_4.2.1+mimiker3_amd64.deb
