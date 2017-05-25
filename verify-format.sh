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
# TODO: Diff matches exclude pattern with target file base name, so rules like
# "tests/*_test_files" won't work. For this reason it's reccomended to run this
# script on a clean source tree - until we come up with a workaround, or diff
# devs stop ignoring their mailing list:
# http://lists.gnu.org/archive/html/bug-diffutils/2016-05/msg00008.html
diff -Naur $THISDIR/ $TMPDIR -X .gitignore -x .git
RES=$?

# Remove temporary directory.
cd $THISDIR
rm -rf $TMPDIR

if ! [ $RES -eq 0 ]
then
    echo "Formatting incorrect for C files!"
    echo "Please run 'make format' before committing your changes,"\
         "or manually apply the changes listed above."
else
    echo "Formatting correct for C files."
fi

exit $RES
