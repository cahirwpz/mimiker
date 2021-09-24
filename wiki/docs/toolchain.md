# Toolchain

## Local environment

### Container image

You can use container image used by our CI system.

To build latest version of container run:
- `git clone git@github.com:cahirwpz/mimiker.git && cd mimiker`
- `podman build -t mimiker .`
- `podman container run -it --rm --workdir /root/mimiker --hostname mimiker -v /home/user/mimiker:/root/mimiker mimiker /bin/bash`

after last command you are in shell inside container, you can install your
favourite set of packages or just build Mimiker with the following command:
- `make`

All changes are synchronized between local directory `/home/user/mimiker` and
`/root/mimiker` inside container, so you can edit source files using your
favourite editor.

It is not required but it is recommended to use podman in rootless mode. You can
use also docker as drop-in replacement, but then you are on your own with file
permissions inside `/home/user/mimiker` directory.

Note: building container is *SLOW* and IO *INTENSIVE* process.

### Old-style local toolchain

If you don't want to use containers you can still build binaries by hand &
create packages - currently we support only .deb package format and latest
Debian stable distribution.

For gnu toolchain - C compiler and set of tools required for build & debug - you
need to invoke `make` command inside `toolchain/gnu` directory.

For qemu - emulator used for local development - run `make` command inside
`toolchain/qemu-mimiker` directory.

That's all - if you have problems look into `Dockerfile` in top directory. It
contains list of packages & commands required to prepare toolchain.
