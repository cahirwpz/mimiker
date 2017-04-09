#!/bin/bash

file="test_initrd.cpio"

function gen_file()
{
    path=$1
    name=$2
    mkdir -p "$path"
    echo "This is the content of file $name in directory $path!" > "$path/$name"
}

mkdir -p initrd

gen_file "initrd/directory1" "file1"
gen_file "initrd/directory1" "file2"
gen_file "initrd/directory1" "file3"
gen_file "initrd/directory2" "file1"
gen_file "initrd/directory2" "file2"
gen_file "initrd/directory2" "file3"
gen_file "initrd" "random_file.txt"
gen_file "initrd" "some_file.exe"
gen_file "initrd" "just_file.cpp"
gen_file "initrd" "some_file.c"
gen_file "initrd" "just_file.hs"
gen_file "initrd/very/very/very/very/very/very/very/very/very/deep/directory" \
  "deep_inside"
gen_file "initrd/short_file_name" "a"
gen_file "initrd/level1/level2/level3" "level123"
gen_file "initrd/level1/level2" "level12"
gen_file "initrd/level1" "level1"

cd initrd 
find -depth -print | cpio --format=crc -o > "../${file}"
cd ..
echo "initial ramdisk built in ${file}"

rm -r initrd

