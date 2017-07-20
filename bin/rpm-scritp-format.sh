#! /bin/sh -
pwd=$PWD
echo $pwd
fromdos -f $pwd/build.sh
fromdos -f $pwd/build_8996.sh
fromdos -f $pwd/build_common.py
fromdos -f $pwd/../tools/build/scons/SCons/scons.sh
fromdos -f $pwd/../core/boot/ddr/build/msm8996.sconscript
