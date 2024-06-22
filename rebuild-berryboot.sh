#!/bin/sh

set -e

BUILDROOT=buildroot-2018.08

# Force rebuild of BerrybootGUI
[ -e $BUILDROOT/output/build/berrybootgui2-1.0 ] && make -C $BUILDROOT berrybootgui2-dirclean

./build-berryboot.sh "$@"
