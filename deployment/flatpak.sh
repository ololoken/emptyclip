#!/bin/bash

function build() {
	branch=$1
	echo "building branch $branch"

	pkg=$base.flatpak
	if [ "$branch" != "master" ]; then
		pkg=${base}_${branch}.flatpak
	fi

	flatpak-builder --default-branch="$branch" --force-clean --repo="$repo_path" --state-dir=flatpak-state flatpak-build flatpak.yml
	flatpak build-bundle "$repo_path" "out/$pkg" "io.gitlab.jazztickets.$project" "$branch"
	rm -rf flatpak-build flatpak-state out/src.tar.gz
}

# includes
source common.inc

# parameters
repo_path="$1"
if [ -z "$repo_path" ]; then
	echo "Usage: ./$(basename "$0") repo_path"
	exit 1
fi

# setup
mkdir -p out

# variables
base=$project-$version-$gitver

# make src package
./make_src.sh
cp "out/$base-src.tar.gz" "out/src.tar.gz"

# build
build master
