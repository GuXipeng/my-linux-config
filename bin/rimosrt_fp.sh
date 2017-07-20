#! /bin/bash
adb root
adb shell  umount /firmware
adb shell mount -t vfat /dev/block/mmcblk0p1 /firmware
adb push goodixfp.b00 /firmware/image/goodixfp.b00
adb push goodixfp.b01 /firmware/image/goodixfp.b01
adb push goodixfp.b02 /firmware/image/goodixfp.b02
adb push goodixfp.b03 /firmware/image/goodixfp.b03
adb push goodixfp.b04 /firmware/image/goodixfp.b04
adb push goodixfp.b05 /firmware/image/goodixfp.b05
adb push goodixfp.b06 /firmware/image/goodixfp.b06
adb push goodixfp.mdt /firmware/image/goodixfp.mdt
