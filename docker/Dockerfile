FROM debian:stretch-backports

WORKDIR /root

ADD http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.2_amd64.deb \
    mipsel-mimiker-elf_1.2_amd64.deb
ADD http://mimiker.ii.uni.wroc.pl/download/qemu-system-mimiker_2.12.0-1_amd64.deb \
    qemu-system-mimiker_2.12.0-1_amd64.deb
COPY requirements.txt .

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
      git make cpio socat curl ctags cscope less locales patch file \
      vim-nox tmux tig python3-pip clang-format-6.0 \
      libsdl2-2.0-0 libglib2.0-0 libpixman-1-0 libpython2.7
RUN dpkg -i mipsel-mimiker-elf_1.2_amd64.deb \
            qemu-system-mimiker_2.12.0-1_amd64.deb
RUN pip3 install -r requirements.txt
RUN echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen && \
    locale-gen && update-locale LANG="en_US.UTF-8"
RUN touch /root/.gitconfig && \
    echo "add-auto-load-safe-path /root/mimiker/.gdbinit" > /root/.gdbinit
