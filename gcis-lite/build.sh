#!/usr/bin/env bash
if [ ! -d "external/sdsl-lite" ]; then
  mkdir -p external/sdsl-lite
  git clone https://github.com/simongog/sdsl-lite.git external/sdsl-lite
fi
./external/sdsl-lite/install.sh .

source set_project_functions.sh

gcis-config