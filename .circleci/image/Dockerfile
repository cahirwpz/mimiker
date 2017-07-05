FROM debian:9.0

WORKDIR /root

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends git make wget cpio socat \
      ctags cscope python3-pip clang-format-3.8 rsync curl \
      libpython2.7 gcc libc6-dev qemu-system-mips seabios openssh-client
RUN pip3 install -I pexpect pep8
RUN wget http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.2_amd64.deb
RUN dpkg -i mipsel-mimiker-elf_1.2_amd64.deb
RUN ln -s /usr/bin/clang-format-3.8 /usr/local/bin/clang-format
