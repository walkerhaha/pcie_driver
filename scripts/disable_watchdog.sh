echo 0 > /proc/sys/kernel/watchdog_thresh
cat  /proc/sys/kernel/watchdog_thresh
# 禁用 nmi_watchdog
echo 0 > /proc/sys/kernel/nmi_watchdog
cat  /proc/sys/kernel/nmi_watchdog

# 禁用 RCU stall 检测

echo 0 > /sys/module/rcupdate/parameters/rcu_cpu_stall_timeout
cat      /sys/module/rcupdate/parameters/rcu_cpu_stall_timeout

echo 0 > /sys/module/rcupdate/parameters/rcu_task_stall_timeout
cat  /sys/module/rcupdate/parameters/rcu_task_stall_timeout

# 增加 stall 检测超时时间（秒）
#echo 300 > /sys/module/rcupdate/parameters/rcu_cpu_stall_timeout

# 禁用 stall panic
#echo 0 > /sys/module/rcupdate/parameters/rcu_cpu_stall_panic

# 抑制 stall 警告
echo 1 > /sys/module/rcupdate/parameters/rcu_cpu_stall_suppress

echo 0 > /proc/sys/kernel/hung_task_timeout_secs
cat  /proc/sys/kernel/hung_task_timeout_secs
