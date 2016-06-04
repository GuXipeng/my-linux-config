adb shell logcat -v time -f /dev/kmsg | adb shell cat /proc/kmsg | tee kernel_logcat.txt
