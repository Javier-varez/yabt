#!/usr/bin/env bash

# Bootstraping process for the Yabt build system. Yabt is self-hosted, but initially it needs to
# be built from another build system. This script performs the following builds:
#
# - stage0: build yabt using the root `makefile`. The outupt will be under ./build/yabt
# - stage1: build yabt using the yabt tool generated in stage0. The output is generated under BUILD
# - stage2: verify the output of the previous step by building yabt using the output of stage1.
#
# The output of stage2 is our final binary

set -e

make clean
rm -rf BUILD_STAGE1
rm -rf BUILD_STAGE2
rm -f yabt

echo 'Building stage0'
make -j
echo 'stage0 completed'

echo 'Building stage1'
./build/yabt -v build --build-dir=BUILD_STAGE1
echo 'stage1 completed'

echo 'Building stage2'
./BUILD_STAGE1/src/yabt/yabt -v build --build-dir=BUILD_STAGE2
echo 'stage2 completed'

cp ./BUILD_STAGE2/src/yabt/yabt yabt
