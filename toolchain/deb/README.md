Toolchain packaging procedure
---

 1. Toolchain should be installed in `${HOME}/local/$TARGET`.
 2. Create temporary directory named `<package-name>-<version>` and enter 
 3. In this directory launch `dh-make -n`,  choose indep
 4. Copy toolchain here `cp ${HOME}/local/$TARGET . -r`
 5. Copy installation script
 `cp ${MIMIKER_TOP}/toolchain/deb/install debian/install`
6. run `debuild -b`
7. deb package should appear outside current directory