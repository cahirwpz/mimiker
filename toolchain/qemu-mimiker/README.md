QEMU for Mimiker kernel development
---

Packages required to build:

 * debhelper
 * ninja-build
 * quilt
 * libglib2.0-dev
 * libsdl2-dev
 * zlib1g-dev

Just issue `make` and you'll get a Debian package in parent directory.

*IMPORTANT!* If you update the package please change `debian/changelog` as well.
