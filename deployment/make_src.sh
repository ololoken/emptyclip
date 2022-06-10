#!/bin/bash

# includes
source common.inc

# variables
base="$project-$version-$gitver"
pkg="$base-src.tar.gz"

# prepare
mkdir -p out

echo "Making $pkg"

# build package
tar --transform "s|^|$base/|" -czvf "out/$pkg" -C ../ \
--exclude="$pkg" \
--exclude=*.swp \
--exclude=.git \
--exclude=working/"$project"* \
src/ \
working/ \
deployment/emptyclip{,.desktop,.png} \
cmake/ \
build.sh \
CMakeLists.txt \
README \
CHANGELOG \
LICENSE

# output
echo -e "\nMade $pkg.gz"
