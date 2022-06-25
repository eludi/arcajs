#!/bin/sh
# arcajs bootstrap script for Linux on x86_64, tested under Ubuntu 18.04 & 20.04

sudo apt -y install libgl1-mesa-dev libudev-dev libxext-dev libasound2-dev libsamplerate-dev libsndio-dev libdbus-1-dev libxext-dev
wget https://www.libsdl.org/release/SDL2-2.0.20.tar.gz
tar xvfz SDL2-2.0.20.tar.gz
mv SDL2-2.0.20 SDL2
cd SDL2 && mkdir build && cd build
../configure --disable-shared --disable-video-wayland
make
mkdir ../lib
mkdir ../lib/Linux_x86_64
cp build/.libs/libSDL2.a ../lib/Linux_x86_64
make clean
cd ../..

sudo apt -y install libcurl4-openssl-dev
wget https://github.com/eludi/arcajs/archive/master.zip
unzip master.zip
mv arcajs-master arcajs
cd arcajs
make
