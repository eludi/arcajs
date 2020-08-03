#!/bin/sh
# arcajs bootstrap script for Raspberry Pi Zero (and likely others) on Raspberry Pi OS

sudo apt -y install libudev-dev libasound2-dev libsamplerate-dev libsndio-dev libdbus-1-dev
wget https://www.libsdl.org/release/SDL2-2.0.12.tar.gz
tar xvfz SDL2-2.0.12.tar.gz
mv SDL2-2.0.12 SDL2
cd SDL2 && mkdir build && cd build
../configure --disable-shared --disable-pulseaudio --disable-esd --disable-video-wayland --disable-video-x11 --disable-video-opengl
make
mkdir ../lib
mkdir ../lib/Linux_armv6l
cp build/.libs/libSDL2.a ../lib/Linux_armv6l
make clean
cd ../..

sudo apt -y install libcurl4-openssl-dev upx-ucl
wget https://github.com/eludi/arcajs/archive/master.zip
unzip master.zip
mv arcajs-master arcajs
cd arcajs
make
upx ./arcajs
