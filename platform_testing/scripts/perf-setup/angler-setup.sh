if [[ "`id -u`" -ne "0" ]]; then
  echo "WARNING: running as non-root, proceeding anyways..."
fi

stop thermal-engine
stop perfd

echo -n 0 > /sys/devices/system/cpu/cpu0/online
echo -n 0 > /sys/devices/system/cpu/cpu1/online
echo -n 0 > /sys/devices/system/cpu/cpu2/online
echo -n 0 > /sys/devices/system/cpu/cpu3/online

echo -n 1 > /sys/devices/system/cpu/cpu4/online
echo -n performance > /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor

echo -n 1 > /sys/devices/system/cpu/cpu5/online
echo -n performance > /sys/devices/system/cpu/cpu5/cpufreq/scaling_governor

echo -n 0 > /sys/devices/system/cpu/cpu6/online
echo -n 0 > /sys/devices/system/cpu/cpu7/online

echo performance > /sys/class/kgsl/kgsl-3d0/devfreq/governor

echo 0 > /sys/class/kgsl/kgsl-3d0/bus_split
echo 1 > /sys/class/kgsl/kgsl-3d0/force_clk_on

echo 10000 > /sys/class/kgsl/kgsl-3d0/idle_timer

