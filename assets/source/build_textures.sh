#!/bin/bash

# get parameters
dir="$1"
if [ -z "$dir" ]; then
	echo "Usage: ./$(basename "$0") texture_dir"
	exit 1
fi

# set up
PACK="../../ext/ae/scripts/pack.py"
mkdir -p ../../working/textures

# pack textures
$PACK ./ "$dir"
pack=$(basename "$dir")
mv -v "$pack.bin" "../../working/textures/$pack"
