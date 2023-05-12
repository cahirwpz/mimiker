#!/bin/bash

MODIFY=-n
if [ "$1" == "--fix" ]; then
  MODIFY=
fi

source <(make dump-tools)

CFILES=$(find -name "*.[ch]" | grep -vf .format-exclude)

$FORMAT $MODIFY --Werror -i $CFILES

if [ $? -ne 0 ]; then
  echo ""
  echo "C files are not formatted correctly."
  echo "If you want to format them automatically please use 'verify-format.sh --fix'"
  echo ""
  echo "If you see this message in CI report consider using our git hook 'scripts/pre-commit'."
  echo "It could be installed using git-setup.sh script."
  exit 1
fi

echo "Formatting correct for C files."
