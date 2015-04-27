#!/bin/sh

if [[ ! -e gcc-linaro-arm-linux-gnueabihf-4.7-2012.11-20121123_linux.tar.bz2 ]]; then
    wget https://launchpad.net/linaro-toolchain-binaries/trunk/2012.11/+download/gcc-linaro-arm-linux-gnueabihf-4.7-2012.11-20121123_linux.tar.bz2
fi
if [[ ! -e gcc-linaro-arm-linux-gnueabihf-4.7-2012.11-20121123_linux ]]; then
    tar xjf gcc-linaro-arm-linux-gnueabihf-4.7-2012.11-20121123_linux.tar.bz2
fi
