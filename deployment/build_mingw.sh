#!/bin/bash

# bash options
set -e

# includes
source common.inc

# get base project dir
projectdir=$(git rev-parse --show-toplevel)
if [ -z "$projectdir" ]; then
	echo "No .git directory found"
	exit 1
fi

# build out dir
outputdir="$projectdir/deployment/out"
mkdir -p "$outputdir"

build() {

	bits=$1

	# get mingw prefix
	if [ "$bits" -eq "32" ]; then
		arch=i686-w64-mingw32
	else
		arch=x86_64-w64-mingw32
	fi

	# run cmake
	builddir="$projectdir/build/mingw$bits"
	mkdir -p "$builddir"
	pushd "$builddir" || exit
	cmake -GNinja -DCMAKE_TOOLCHAIN_FILE="../../cmake/mingw${bits}.cmake" -DCMAKE_BUILD_TYPE=Release ../../

	# build
	if ! ninja; then
		echo "failed $builddir"
		exit
	fi

	# go back to deployment dir
	popd || exit

	# create new working dir
	archive_base=${project}-${version}-${gitver}-win${bits}
	archive=${archive_base}.zip
	rm -rf "${archive_base}"
	cp -r "${projectdir}/working" "${archive_base}"
	rm "${projectdir}/working/${project}.exe"
	rm -f "${archive_base}/maps/test.map.gz"

	# remove linux only files
	rm -f "${archive_base}"/"${project}"{,_debug*,_reldeb}

	# build files to copy
	libs=(
		OpenAL32.dll
		SDL2.dll
		SDL2_image.dll
		libbrotlicommon.dll
		libbrotlidec.dll
		libbz2-1.dll
		libfreetype-6.dll
		libgcc_s_seh-1.dll
		libogg.dll
		libpng16-16.dll
		libsharpyuv.dll
		libsqlite3.dll
		libssp-0.dll
		libstdc++-6.dll
		libvorbis-0.dll
		libvorbisfile-3.dll
		libwebp.dll
		libwebpdemux.dll
		libwinpthread-1.dll
		zlib1.dll
	)

	# copy dlls
	pushd "/usr/$arch/bin/" || exit
	cp "${libs[@]}" "${projectdir}/deployment/${archive_base}/"
	popd || exit

	# strip exe
	${arch}-strip "${archive_base}"/${project}.exe

	# copy files
	cp "${projectdir}"/{README,CHANGELOG} "${archive_base}"/
	echo "${project}.exe -level bench" > "${archive_base}"/run_bench.bat
	echo "${project}.exe -bench b01" > "${archive_base}"/run_b01.bat
	chmod +x "${archive_base}"/*.bat

	# zip
	zip -r "${archive}" "${archive_base}"

	# clean up
	rm -rf "${archive_base}"
	mv "$archive" "$outputdir"
}

# remove old zips
rm -f "$outputdir"/"${project}-${version}"*.zip

# build project
build 64
