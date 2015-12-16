	This directory contains the build script for smallclib standard C
library (taken from toolchain's resources), along with a tiny glue
layer.

	 The build script does not compile the entire library, instead, a
list of used functions is explicitly listed in the Makefile. If you
wish to use another function, add the corresponding file to the
SOURCES in the Makefile. Then observe linker errors, and append files
that provide missing symbols to SOURCES.
