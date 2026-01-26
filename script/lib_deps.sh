#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

if [ ! -d "$SCRIPT_DIR"/../deps/cstd ]; then
  git clone https://github.com/jmatth11/cstd.git "$SCRIPT_DIR"/../deps/cstd
  cd "$SCRIPT_DIR"/../deps/cstd
  ./install_deps.sh
  make
  zig build -Doptimize=ReleaseSafe
  cd -
fi
