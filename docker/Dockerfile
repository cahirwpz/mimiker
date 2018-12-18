FROM debian:stretch-backports

# XXX: Create a group in which we will operate with a hardcoded GID,
# so that we are able to set proper directory permissions on host
# for volume with source code.
RUN addgroup --gid 1024 mimiker && \
    adduser --disabled-password --gecos "" --force-badname \
    --ingroup mimiker --ingroup sudo mimiker

WORKDIR /home/mimiker

ADD http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.3_amd64.deb \
    mipsel-mimiker-elf_1.3_amd64.deb
ADD http://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_2.12.0+mimiker1_amd64.deb \
    qemu-mimiker_2.12.0+mimiker1_amd64.deb
ADD http://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_1.3_amd64.deb \
    aarch64-mimiker-elf_1.3_amd64.deb
COPY requirements.txt .

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
        git make cpio curl exuberant-ctags cscope rsync socat \
        gcc less locales patch file sudo zsh vim-nox tmux tig ack \
        libc6-dev python3-pip clang-format-6.0 \
        libfdt1 libpython3.5 libsdl2-2.0-0 libglib2.0-0 libpixman-1-0

# rsync required by verify-format.sh
# gcc & libc6-dev required by newlib-3.0.0
# socat required by launch

RUN dpkg -i mipsel-mimiker-elf_1.3_amd64.deb \
            aarch64-mimiker-elf_1.3_amd64.deb \
            qemu-mimiker_2.12.0+mimiker1_amd64.deb
RUN pip3 install -r requirements.txt
RUN echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen && \
    locale-gen && update-locale LANG="en_US.UTF-8"
RUN touch .gitconfig && \
     echo "add-auto-load-safe-path ~/mimiker/.gdbinit" > .gdbinit && \
     echo "mimiker  ALL=(ALL)   NOPASSWD: ALL" >> /etc/sudoers

USER mimiker
# ADD https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh \
#     zsh-install.sh
# RUN bash zsh-install.sh
