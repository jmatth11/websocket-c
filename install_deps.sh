# install zig dependency for zig plugin.
which zig > /dev/null
if [ $? -ne 0 ]; then
  sudo apt install -y snapd
  sudo snap install zig --classic --beta
fi

if [ ! -d ./deps/cstd ]; then
  git clone https://github.com/jmatth11/cstd.git deps/cstd
  cd deps/cstd
  make
  zig build
  cd -
fi
