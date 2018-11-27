#!/bin/sh
dtc -I dts -O dtb -o device_hints.dtb device_hints.dts
python generate_dev_hints.py device_hints.dtb > device_hints.c

