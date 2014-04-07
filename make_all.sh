#! /bin/bash

#
# Try to build all Makefiles
#    It should be improved to, at the end, print only errors or warnings
#

for i in $(find . -iname Makefile | grep -v TinyOS); do
    make -s -C $(dirname $i) clean  --no-print-directory   RADIO=WITH_CC1101
    make -s -C $(dirname $i) --no-print-directory   RADIO=WITH_CC1101 || { echo $i; echo ; echo ; }
    make -s -C $(dirname $i) clean  --no-print-directory   RADIO=WITH_CC1101
done
