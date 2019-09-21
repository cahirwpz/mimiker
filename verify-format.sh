#!/bin/sh

PAGER=cat

# Use make format to cleanup the copied tree
make format > /dev/null

# See if there are any changes compared to checked out sources.
if ! git diff --check --exit-code >/dev/null; then
    echo "Formatting incorrect for C files!"
    echo "Please run 'make format' before committing your changes,"
    echo "or manually apply the changes listed above."
    git diff
    exit 1
fi

echo "Formatting correct for C files."
