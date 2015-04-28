#!/bin/sh

if [[ ! -e gcc-linaro-arm-linux-gnueabihf-4.7-2012.11-20121123_linux.tar.bz2 ]]; then
    wget https://launchpad.net/linaro-toolchain-binaries/trunk/2012.11/+download/gcc-linaro-arm-linux-gnueabihf-4.7-2012.11-20121123_linux.tar.bz2
fi
if [[ ! -e gcc-linaro-arm-linux-gnueabihf-4.7-2012.11-20121123_linux ]]; then
    tar xjf gcc-linaro-arm-linux-gnueabihf-4.7-2012.11-20121123_linux.tar.bz2
fi
if [[ ! -e buildroot-2013.11.tar.gz ]]; then
    wget http://buildroot.uclibc.org/downloads/buildroot-2013.11.tar.gz
fi
if [[ ! -e buildroot-2013.11 ]]; then
    tar xf buildroot-2013.11.tar.gz
fi
if [[ ! -e bootloader.img ]]; then
    wget https://dl.dropboxusercontent.com/u/22450509/fpga-series/bootloader.img
fi
# if [[ ! -e soc_system_13_0_0_06252013_90253.tar.gz ]]; then
#     wget http://www.rocketboards.org/pub/Projects/SoCKitLinaroLinuxDesktop/soc_system_13_0_0_06252013_90253.tar.gz
# fi
# 
# SOCDIR="soc_system_13_0_0_06252013_90253"
# if [[ ! -e "$SOCDIR" ]]; then
#     mkdir "$SOCDIR"
#     cd "$SOCDIR"
#     tar xf ../soc_system_13_0_0_06252013_90253.tar.gz
#     cd ../
# fi
# if [[ ! -e linaro-quantal-nano-20130422-342.tar.gz ]]; then
#     wget https://releases.linaro.org/13.04/ubuntu/quantal-images/nano/linaro-quantal-nano-20130422-342.tar.gz
# fi
# LINARO="linaro-quantal-nano-20130422-342"
# if [[ ! -e "$LINARO" ]]; then
#     mkdir "$LINARO"
#     cd "$LINARO"
#     tar xf ../linaro-quantal-nano-20130422-342.tar.gz
#     cd ../
# fi

