#!/bin/bash

set -e

THISSCRIPT=$(basename $0)
RULEFILES=$(find . -name initrd.rules)
BUILDDIR=$(mktemp -d)
DEPFILE=".initrd.D"
CPIOFILE="initrd.cpio"

### TODO: It is possible to implement this script so that it would only copy the
### modified files, and incrementally update the ramdisk. However, this is
### difficult, and we don't keep thousands of files in the ramdisk, so
### recreating entire cpio archive will be good for now.

SOURCEFILES=

# Gather all files with rules, and copy files to the build dir
for rulefile in $RULEFILES; do
    echo "Processing file $rulefile"
    sourcedir=$(dirname $rulefile)
    while read -r line; do
        source=$sourcedir/$(echo "$line" | cut  -d' ' -f1)
        target=$BUILDDIR/$(echo "$line" | cut  -d' ' -f2)
        echo "$source -> $target"
        mkdir -p $(dirname $target)
        cp -r $source $target
        SOURCEFILES+=" $source"
    done < $rulefile
done

# Create cpio archive
echo "Generating $CPIOFILE..."
(cd $BUILDDIR && find -depth -print | cpio --format=crc -o) > "$CPIOFILE"

# Generate depfile
rm -f $DEPFILE
echo "initrd.cpio: $THISSCRIPT $RULEFILES $SOURCEFILES" | tr '\n' ' ' >> $DEPFILE

# Remove the temporary dir.
rm -rf $BUILDDIR
