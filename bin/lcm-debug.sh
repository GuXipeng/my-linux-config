 adb shell "echo 'file mdss_dsi_panel.c +pmflt' > /sys/kernel/debug/dynamic_debug/control"
 adb shell "echo 'file mdss_dsi_clk.c +pmflt' > /sys/kernel/debug/dynamic_debug/control"
 adb shell "echo 'file mdss_dsi.c +pmflt' > /sys/kernel/debug/dynamic_debug/control"
 adb shell "echo 'file mdss_fb.c +pmflt' > /sys/kernel/debug/dynamic_debug/control"

