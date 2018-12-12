FROM debian:stretch-backports

WORKDIR /root

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends git make wget cpio socat \
      ctags cscope python3-pip clang-format-6.0 rsync curl gcc libc6-dev \
      libpython2.7 openssh-client libsdl2-2.0-0 libglib2.0-0 libpixman-1-0 \
      locales vim-nox tmux tig
RUN pip3 install -I pexpect pep8
RUN wget http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.2_amd64.deb
RUN dpkg -i mipsel-mimiker-elf_1.2_amd64.deb
RUN wget http://mimiker.ii.uni.wroc.pl/download/qemu-system-mimiker_2.12.0-1_amd64.deb
RUN dpkg -i qemu-system-mimiker_2.12.0-1_amd64.deb
RUN touch root/.gitconfig && \
    echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen
RUN locale-gen

