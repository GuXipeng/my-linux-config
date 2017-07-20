#!/bin/sh -
echo 当前屏信息如下:

adb root

adb shell cat proc/cmdline

#echo byte_clk如下注：dsi_clk = byte_clk * lane_num

adb shell cat /sys/kernel/debug/clk/*mdss_byte0_clk/measure

echo clk_rate如下

adb shell cat /sys/kernel/debug/mdss_panel_fb0/intf0/clk_rate

echo 當前幀率如下：

adb shell cat /sys/devices/virtual/graphics/fb0/dynamic_fps

echo 当前背光LCD-BCAKLIGHT：

adb shell cat /sys/class/leds/lcd-backlight/brightness

echo 当前背光WLED：

adb shell cat /sys/class/leds/wled/brightness
