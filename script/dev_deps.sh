#!/usr/bin/env bash

# install zig dependency for zig plugin.
which zig > /dev/null
if [ $? -ne 0 ]; then
  sudo apt install -y snapd
  sudo snap install zig --classic --beta
fi

PKGS_TO_INSTALL=""

# check for packages
which pkg-config > /dev/null
if [ $? -ne 0 ]; then
  PKGS_TO_INSTALL="pkg-config $PKGS_TO_INSTALL"
fi
dpkg -s libssl-dev > /dev/null 2>&1
if [ $? -ne 0 ]; then
  PKGS_TO_INSTALL="libssl-dev $PKGS_TO_INSTALL"
fi

# install packages
if [ -n "$PKGS_TO_INSTALL" ]; then
  sudo apt install -y "PKGS_TO_INSTALL"
fi
