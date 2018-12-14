FROM debian:stretch-backports

WORKDIR /root

COPY requirements.txt .

RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
      git make cpio curl exuberant-ctags cscope rsync socat \
      gcc libc6-dev python3-pip clang-format-6.0 \
      libpython3.5 libsdl2-2.0-0 libglib2.0-0 libpixman-1-0
# rsync required by verify-format.sh
# gcc & libc6-dev required by newlib-3.0.0
# socat required by launch
RUN pip3 install -r requirements.txt
RUN curl -O http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.3_amd64.deb && \
    dpkg -i mipsel-mimiker-elf_1.3_amd64.deb && rm -f mipsel-mimiker-elf_1.3_amd64.deb 
RUN curl -O http://mimiker.ii.uni.wroc.pl/download/qemu-system-mimiker_2.12.0-1_amd64.deb && \
    dpkg -i qemu-system-mimiker_2.12.0-1_amd64.deb && rm -f qemu-system-mimiker_2.12.0-1_amd64.deb
