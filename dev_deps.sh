#!/usr/bin/env bash

# install zig dependency for zig plugin.
which zig > /dev/null
if [ $? -ne 0 ]; then
  sudo apt install -y snapd
  sudo snap install zig --classic --beta
fi

sudo apt install -y pkg-config libssl-dev
