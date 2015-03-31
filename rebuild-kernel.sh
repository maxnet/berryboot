#!/bin/sh

set -e

# Force rebuild of Linux kernel
(cd buildroot-2015.02 ; make linux-dirclean)
./build-berryboot.sh "$@"

