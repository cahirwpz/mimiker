#!/bin/sh
./convert_fdt_to_c_array.py ../drv/device_hints.dts
mv device_hints.c ../drv/device_hints.c

