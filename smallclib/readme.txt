	This directory contains the smallclib standard C library, along with a
tiny glue layer.

	 ./smallclib has the original, unmodified sources, pulled straight
from toolchain resources. It should be easy to convert that directory
into a submodule, symlink, or a cross-reference, or replace it with
another version.

	 The build script does not compile the entire library, instead, a
list of used functions is explicitly listed in the Makefile. If you
wish to use another function, add the corresponding file to the
SOURCES in the Makefile. Then observe linker errors, and append files
that provide missing symbols to SOURCES.
