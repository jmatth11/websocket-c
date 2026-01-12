#!/usr/bin/env bash

if [ ! -d ./deps/cstd ]; then
  git clone https://github.com/jmatth11/cstd.git deps/cstd
  cd deps/cstd
  ./install_deps.sh
  make
  zig build -Doptimize=ReleaseSafe
  cd -
fi
