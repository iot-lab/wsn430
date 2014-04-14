#! /bin/bash

#
# Try to build all Makefiles
#    It should be improved to, at the end, print only errors or warnings
#

for i in $(find . -iname Makefile | grep -v TinyOS | grep -v drivers/Makefile); do
    make -s -C $(dirname $i) clean  --no-print-directory   RADIO=WITH_CC1101
    result=$(make -s -C $(dirname $i) --no-print-directory   RADIO=WITH_CC1101 2>&1)

    if [[ "$result" != "" ]]; then
        echo $i
        echo "$result"
        echo
        echo
    fi
    make -s -C $(dirname $i) clean  --no-print-directory   RADIO=WITH_CC1101
done
