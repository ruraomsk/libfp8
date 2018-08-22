#!/bin/bash
# 
# File:   copyfp8.bash
# Author: rusin
#
# Created on 08.05.2018, 9:23:06
#
echo Make if need dirs
mkdir /usr/local/include/fp8
mkdir /usr/local/include/fp8/drivers
echo copy include files for lib
cp *.h /usr/local/include/fp8
cp drivers/*.h /usr/local/include/fp8/drivers
echo copy control files for lib
cp libfp8.pc /usr/lib/pkgconfig
cp 99fp8 /etc/env.d
env-update && source /etc/profile
echo Lib for FP8 CPC108 installed...
