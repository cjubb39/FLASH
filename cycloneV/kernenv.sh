#!/bin/sh

UBOOT="$PWD/u-boot-socfpga/tools"
if [[ -e "$UBOOT" ]]; then
    if [[ ! "$PATH" == *"$UBOOT"* ]]; then
        export ARCH=arm
        export CROSS_COMPILE="$PWD/gcc-linaro-arm-linux-gnueabihf-4.7-2012.11-20121123_linux/bin/arm-linux-gnueabihf-"
        export LOADADDR=0x8000
        export PATH="$UBOOT:$PATH"
    else
        unset ARCH
        unset CROSS_COMPILE
        unset LOADADDR
        export PATH="${PATH#$UBOOT:}"
    fi
fi

