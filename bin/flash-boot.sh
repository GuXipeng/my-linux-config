#! /bin/sh
adb reboot-bootloader
sudo fastboot flash boot boot.img reboot
