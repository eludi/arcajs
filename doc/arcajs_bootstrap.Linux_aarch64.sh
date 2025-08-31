#!/bin/sh
# experimental arcajs bootstrap script for cross-compiling to Linux aarch64, tested under Ubuntu 22.04 as host and muOS and rocknix as target

wget https://www.libsdl.org/release/SDL2-2.32.8.tar.gz
tar xvfz SDL2-2.32.8.tar.gz
mv SDL2-2.32.8 SDL2
cd SDL2 && mkdir build && cd build
../configure --disable-static --disable-altivec --disable-oss --disable-jack --disable-jack-shared --disable-esd --disable-esd-shared \
  --disable-arts --disable-arts-shared --disable-nas --disable-nas-shared --disable-libsamplerate-shared --disable-sndio \
  --disable-video-wayland --disable-wayland-shared --disable-video-wayland-qt-touch --disable-video-vivante --disable-video-x11 \
  --disable-video-vulkan --disable-video-opengl --enable-video-opengles --disable-video-opengles1 --enable-video-opengles2
make
mkdir ../lib
mkdir ../lib/Linux_aarch64
cp build/.libs/libSDL2* ../lib/Linux_aarch64
make clean
cd ../..

wget https://github.com/eludi/arcajs/archive/master.zip
unzip master.zip
mv arcajs-master arcajs
cd arcajs
make
