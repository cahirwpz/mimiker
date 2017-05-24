#!/bin/bash

find . \( -name 'cache' -prune \) -o \( -name '.*.py' -prune \)\
  -o \( -name '*.py' -print \) |xargs pep8
RES=$?


if ! [ $RES -eq 0 ]
then
    echo "Formatting incorrect. Please manually apply the changes" \
         "listed above."
else
    echo "Formatting correct."
fi

exit $RES
