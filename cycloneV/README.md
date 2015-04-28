# Building the Kernel

1. Initialize and update the "linux-socfpga" and "u-boot-socfpga" submodules. They are already checked out to their proper branch.
2. ./pullbuild.sh to get the toolchain to build the kernel
3. source kernenv.sh to establish the proper enviroment variables and setup path (source it again to undo the changes)
4. make and be on your merry way
