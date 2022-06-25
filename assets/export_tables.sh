#!/bin/bash
echo "unused"
exit

# cd to script dir
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$script_dir" || exit

# make temp dir
mkdir -p temp

# convert files to csv
if pgrep soffice.bin >/dev/null; then
	echo "LibreOffice must be closed first!"
	exit 1
fi
libreoffice --headless --convert-to csv tables/{armor.ods,itemdrops.ods,monsters.ods,weapons.ods} --outdir temp/

# convert from csv to tsv
find temp/ -iname "*.csv" -exec sh -c 'f="$1"; f="${f%.*}"; sed -i "s/\r//g" "${f}.csv"; sed -E -f "scripts/csv2tsv.sed" "${f}.csv" > "${f}.tsv"' shell "{}" \;

# move files
mv temp/*.tsv ../working/tables/

# remove temp files
rm temp/*
rm -fd temp
