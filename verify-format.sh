#!/bin/sh

modify=-n
if [ "$1" = "--fix" ]; then
  modify=
fi

CFILES=$(find -name "*.[ch]" | grep -vf .format-exclude)

clang-format-14 $modify --Werror --style=file -i $CFILES || exit 1

echo "Formatting correct for C files."
