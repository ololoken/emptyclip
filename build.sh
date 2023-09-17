#!/bin/bash

# check programs
for program in "ninja" "cmake"; do
	type $program >/dev/null 2>&1 || {
		echo >&2 "'$program' is not installed!"
		exit 1
	}
done

# get type
build_type="Release"
if [ -n "$1" ]; then
	build_type="$1"
fi

# get build dir
build_dir=$(echo "$build_type" | tr '[:upper:]' '[:lower:]')

# add suffix
if [ "$ENABLE_SANITIZE" == "1" ]; then
	build_dir="${build_dir}san"
fi

# make
mkdir -p "build/$build_dir"
pushd "build/$build_dir" || exit
cmake -GNinja -DCMAKE_BUILD_TYPE="${build_type}" ../../
ninja
popd || exit
