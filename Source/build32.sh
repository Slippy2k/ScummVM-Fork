#!/bin/bash
chmod +x configure
./configure --host=i686 --disable-debug --enable-release --enable-all-engines --enable-freetype2 --enable-fluidsynth|| exit 1
make clean || exit 1
make -j8 || exit 1





