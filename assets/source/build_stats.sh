#!/bin/bash

# set up
data_dir=../../working/data
db=$data_dir/stats.db

# make dir
mkdir -p "$data_dir"

# build db
rm -f "$db"
for f in stats/*.tsv; do
	./tsv2sqlite.sh "$f" "$db"
done
