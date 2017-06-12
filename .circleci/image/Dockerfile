FROM ubuntu:16.04

ENV PATH="/opt/mipsel-mimiker-elf/bin/:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
RUN apt-get -q update && apt-get install -y git make wget cpio socat qemu-system-mips ctags cscope python3-pip clang-format-3.8 rsync
RUN pip3 install -I pexpect pep8
RUN wget http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.1_amd64.deb
RUN dpkg -i mipsel-mimiker-elf_1.1_amd64.deb
RUN ln -s /usr/bin/clang-format-3.8 /usr/local/bin/clang-format