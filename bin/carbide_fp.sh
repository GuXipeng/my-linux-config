#! /bin/bash
adb root
adb shell  umount /firmware
adb shell mount -t vfat /dev/block/sde11 /firmware
adb push fpctzappfingerprint.b00 /firmware/image/fpctzappfingerprint.b00
adb push fpctzappfingerprint.b01 /firmware/image/fpctzappfingerprint.b01
adb push fpctzappfingerprint.b02 /firmware/image/fpctzappfingerprint.b02
adb push fpctzappfingerprint.b03 /firmware/image/fpctzappfingerprint.b03
adb push fpctzappfingerprint.b04 /firmware/image/fpctzappfingerprint.b04
adb push fpctzappfingerprint.b05 /firmware/image/fpctzappfingerprint.b05
adb push fpctzappfingerprint.b06 /firmware/image/fpctzappfingerprint.b06
adb push fpctzappfingerprint.mdt /firmware/image/fpctzappfingerprint.mdt
