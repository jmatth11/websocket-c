#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

LD_LIBRARY_PATH="$SCRIPT_DIR"/../deps/cstd/lib:"$SCRIPT_DIR"/../deps/cstd/deps/utf8-zig/zig-out/lib ./bin/main
