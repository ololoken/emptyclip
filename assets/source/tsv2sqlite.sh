#!/bin/bash

# check programs
type sqlite3 >/dev/null 2>&1 || {
	echo >&2 "'sqlite3' is not installed!"
	exit 1
}

# parameters
in="$1"
out="$2"
if [ -z "$in" ] || [ -z "$out" ]; then
	echo "Usage: $(basename "$0") input.tsv out.db"
	exit 1
fi

# variables
basename=$(basename "$in")
file=${basename%%.*}

# convert header to array
IFS=$'\t' read -r -a fields <<< "$(head -n1 "$in")"
unset IFS

# build create table sql
sql="${fields[0]} TEXT PRIMARY KEY$(printf ", %s TEXT" "${fields[@]:1}")"
sql="CREATE TABLE ${file}(${sql})";

# create table and import data
sqlite3 "$out" "$sql"
sqlite3 "$out" -cmd ".mode tabs" ".import --skip 1 $in $file"
