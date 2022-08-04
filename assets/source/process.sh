#!/bin/bash

# set pack script
PACK="../../ext/ae/scripts/pack.py"

# set up
mkdir -p ../../working/data
mkdir -p ../../working/textures

# pack textures
for f in textures/*; do
	$PACK ./ "$f"

	pack=$(basename "$f")
	mv -v "$pack.bin" "../../working/textures/$pack"
done

# pack sounds
$PACK ./ sounds
mv -v sounds.bin ../../working/data/sounds
