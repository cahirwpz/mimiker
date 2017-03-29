#!/bin/bash

THISDIR=$PWD
TMPDIR=$(mktemp -d)

# Copy entire working tree to temporary directory, excluding files from
# .gitignore, as well as the .git direcory itself.
rsync -a $THISDIR/ $TMPDIR --exclude-from=.gitignore --exclude=.git

cd $TMPDIR
# Use make format to cleanup the copied tree
make format > /dev/null

# Compare directories, again excluding some ignored files.
diff -Naur $THISDIR/ $TMPDIR -X .gitignore -x .git
RES=$?

# Remove temporary directory.
cd $THISDIR
rm -rf $TMPDIR

if ! [ $RES -eq 0 ]
then
    echo "Formatting incorrect. Please run 'make format' before"  \
         "committing your changes, or manually apply the changes" \
         "listed above."
else
    echo "Formatting correct."
fi

exit $RES
