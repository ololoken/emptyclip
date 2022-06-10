#!/bin/bash

# variables
project=emptyclip
version=$(grep "GAME_VERSION=.*" -o ../CMakeLists.txt | sed 's/[^0-9.]//g')
gitver=$(git log --oneline | wc -l)
