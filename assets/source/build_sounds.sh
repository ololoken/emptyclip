#!/bin/bash

# set up
PACK="../../ext/ae/scripts/pack.py"
mkdir -p ../../working/data

# pack sounds
$PACK ./ sounds
mv -v sounds.bin ../../working/data/sounds
