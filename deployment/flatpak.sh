#!/bin/bash

function build() {
	branch=$1
	echo "building branch $branch"

	pkg=$base.flatpak
	if [ "$branch" != "master" ]; then
		pkg=${base}_${branch}.flatpak
	fi

	CCACHE_DIR=~/.cache/ccache/ flatpak-builder --default-branch="$branch" --force-clean --ccache --disable-cache --repo="$repo_path" --state-dir="$state_path" flatpak-build flatpak.json
	flatpak build-bundle "$repo_path" "out/$pkg" "io.gitlab.jazztickets.$project" "$branch"
	rm -rf flatpak-build out/src.tar.gz
}

# includes
source common.inc

# parameters
repo_path="$1"
state_path="$2"
if [ -z "$repo_path" ] || [ -z "$state_path" ]; then
	echo "Usage: $0 repo_path state_path"
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
