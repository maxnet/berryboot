#!/bin/sh

set -e

BUILDROOT=buildroot-2018.08

# Force rebuild of Linux kernel
make -C $BUILDROOT linux-dirclean
./build-berryboot.sh "$@"

