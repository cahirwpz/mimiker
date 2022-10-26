#!/bin/bash
nocolor='\033[0m'
col_red='\033[0;31m'
col_orange='\033[0;33m'
col_blue='\033[0;34m'

function fatal() {
    echo -e "${col_red}ERROR${nocolor}: "$1
    exit 1
}

function warn() {
    echo -e "${col_orange}WARN${nocolor}: "$1
}

function info() {
    echo -e "${col_blue}INFO${nocolor}: "$1
}

function ask() {
    read -p "Do you want to proceed? (Y/n) " choice
    case $choice in
        [yY] ) return;;
        [nN] ) echo exiting...;
            exit 1;;
        * ) echo invalid response. Assuming NO.;
            exit 1;;
    esac
}

# all checks before script is run
[ "`id -u`" -eq 0 ] && fatal "You are running as root... it is not supported"
[ -f /etc/apt/sources.list.d/llvm.list ] && warn 'You already have llvm sources.list. The script will override it.' && ask
[ -f requirements.txt ] || fatal 'You are not in the Mimiker repo topdir.'
[ -f /usr/bin/apt ] || fatal '/usr/bin/apt is missing; is your distro debian based?'
[ -f /usr/bin/sudo ] || fatal 'sudo is missing; (run `su -c "apt install sudo"`)'

DEBS="git make ccache cpio curl gnupg universal-ctags cscope socat patch gperf quilt byacc python3 python3-pip python3-virtualenv device-tree-compiler tmux lsb-release gdb-multiarch qemu-system-aarch64 qemu-system-riscv64"

info "Install required packages"
info "Upgrade privileges to root user"
info "Install packages from base repository"

sudo -u root sh -c "apt-get update -q && apt install ${DEBS}"

info "Drop privileges to $USER"
info "Install local python dependencies"

python3 -m venv mimiker-env
. mimiker-env/bin/activate
pip3 install -r requirements.txt

LLVM_VERSION=14
DEBS_LLVM="clang-${LLVM_VERSION} clang-format-${LLVM_VERSION} llvm-${LLVM_VERSION} lld-${LLVM_VERSION}"

CODENAME=$(lsb_release -sc 2>/dev/null)
case $CODENAME in
    bullseye|buster)

        info "Prepare sources.list for llvm && install llvm-${LLVM_VERSION}"
        LLVM_REPO="deb http://apt.llvm.org/${CODENAME}/ llvm-toolchain-${CODENAME}-${LLVM_VERSION} main"
        sudo -u root sh -c "apt-key adv --fetch-keys https://apt.llvm.org/llvm-snapshot.gpg.key && echo ${LLVM_REPO} > /etc/apt/sources.list.d/llvm.list && apt-get update -q && apt-get install ${DEBS_LLVM}"

        WORKDIR=$(mktemp -d)
        info "${WORKDIR} created"
        info "Download prebuild toolchain"

        curl -o $WORKDIR/mipsel-mimiker-elf_latest_amd64.deb https://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_latest_amd64.deb
        curl -o $WORKDIR/aarch64-mimiker-elf_latest_amd64.deb https://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_latest_amd64.deb
        curl -o $WORKDIR/riscv32-mimiker-elf_latest_amd64.deb https://mimiker.ii.uni.wroc.pl/download/riscv32-mimiker-elf_latest_amd64.deb
        curl -o $WORKDIR/riscv64-mimiker-elf_latest_amd64.deb https://mimiker.ii.uni.wroc.pl/download/riscv64-mimiker-elf_latest_amd64.deb
        curl -o $WORKDIR/qemu-mimiker_latest_amd64.deb https://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_latest_amd64.deb

        info "Install toolchain"
        sudo -u root sh -c "dpkg -i $WORKDIR/*.deb"

        info "Cleanup $WORKDIR"
        rm -r $WORKDIR
        ;;
    *)
        warn "Your distribution doesn't support our prebuild toolchain"
        ;;
esac

exit 0
