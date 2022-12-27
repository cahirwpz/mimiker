#!/bin/sh

PYFILES=$(find . \( -name 'toolchain' -prune \)\
              -o \( -name '.*.py' -prune \) \
              -o \( -name '*.py' -printf '%P\n' \))
PYEXTRA="launch"

pycodestyle ${PYEXTRA} ${PYFILES}
RES=$?

if ! [ $RES -eq 0 ]
then
    echo "Formatting incorrect for Python files!"
    echo "Please manually fix the warnings listed above."
else
    echo "Formatting correct for Python files."
fi

exit $RES
