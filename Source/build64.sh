#!/bin/bash
chmod +x configure
./configure --host=x86_64-w64-mingw32 --disable-debug --enable-release --enable-all-engines --enable-freetype2 --enable-fluidsynth|| exit 1
make clean || exit 1
make -j3 || exit 1





