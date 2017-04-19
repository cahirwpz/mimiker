#!/bin/bash

dir="initrd_test_files"

function gen_file()
{
    path=$1
    name=$2
    mkdir -p "$path"
    echo "This is the content of file \"$name\" in directory \"$path\"!" > "$path/$name"
}

mkdir -p $dir

gen_file "$dir/directory1" "file1"
gen_file "$dir/directory1" "file2"
gen_file "$dir/directory1" "file3"
gen_file "$dir/directory2" "file1"
gen_file "$dir/directory2" "file2"
gen_file "$dir/directory2" "file3"
gen_file "$dir" "random_file.txt"
gen_file "$dir" "some_file.exe"
gen_file "$dir" "just_file.cpp"
gen_file "$dir" "some_file.c"
gen_file "$dir" "just_file.hs"
gen_file "$dir/very/very/very/very/very/very/very/very/very/deep/directory" "deep_inside"
gen_file "$dir/short_file_name" "a"
gen_file "$dir/level1/level2/level3" "level123"
gen_file "$dir/level1/level2" "level12"
gen_file "$dir/level1" "level1"
