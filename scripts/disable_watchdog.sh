echo 0 > /proc/sys/kernel/watchdog_thresh
cat  /proc/sys/kernel/watchdog_thresh
# 禁用 nmi_watchdog
echo 0 > /proc/sys/kernel/nmi_watchdog
cat  /proc/sys/kernel/nmi_watchdog

# 禁用 RCU stall 检测


echo 0 > /sys/module/rcupdate/parameters/rcu_cpu_stall_timeout
cat /sys/module/rcupdate/parameters/rcu_cpu_stall_timeout
