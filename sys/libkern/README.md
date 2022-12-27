This directory contains the build script for smallclib standard C
library (used parts included in ./smallclib), along with several extra
utilities.

The build script does not compile the entire library, instead, a list
of used functions is explicitly listed in the Makefile. If you wish to
use another function, add the corresponding file to the SOURCES in the
Makefile, and add those files from ImgTech's MIPS SmallCLib to the
./smallclib directory. Then observe linker errors, and append files
that provide missing symbols to SOURCES.
