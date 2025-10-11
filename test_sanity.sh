mydir=`pwd`
echo $mydir

#speed gen4
setpci -s 18:00.0 cap10+30.b=4
setpci -s 17:00.0 3e.b=42
setpci -s 17:00.0 3e.b=2
sleep 10s
lspci -vvvs 18:00.0 | grep Lnk  > sanity_gen4.log
echo 1 > /sys/bus/pci/devices/0000\:18\:00.0/remove
echo 1 > /sys/bus/pci/devices/0000\:18\:00.1/remove
echo 1 > /sys/bus/pci/devices/0000\:17\:00.0/rescan

./test/build/test  "[sanity]" >> sanity_gen4.log

#speed gen3
setpci -s 18:00.0 cap10+30.b=3
setpci -s 17:00.0 3e.b=42
setpci -s 17:00.0 3e.b=2
sleep 10s
lspci -vvvs 18:00.0 | grep Lnk  > sanity_gen3.log
echo 1 > /sys/bus/pci/devices/0000\:18\:00.0/remove
echo 1 > /sys/bus/pci/devices/0000\:18\:00.1/remove
echo 1 > /sys/bus/pci/devices/0000\:17\:00.0/rescan

./test/build/test  "[sanity]" >> sanity_gen3.log

#speed gen2
setpci -s 18:00.0 cap10+30.b=2
setpci -s 17:00.0 3e.b=42
setpci -s 17:00.0 3e.b=2
sleep 10s
lspci -vvvs 18:00.0 | grep Lnk  > sanity_gen2.log
echo 1 > /sys/bus/pci/devices/0000\:18\:00.0/remove
echo 1 > /sys/bus/pci/devices/0000\:18\:00.1/remove
echo 1 > /sys/bus/pci/devices/0000\:17\:00.0/rescan

./test/build/test  "[sanity]" >> sanity_gen2.log

#speed gen1
setpci -s 18:00.0 cap10+30.b=1
setpci -s 17:00.0 3e.b=42
setpci -s 17:00.0 3e.b=2
sleep 10s
lspci -vvvs 18:00.0 | grep Lnk  > sanity_gen1.log
echo 1 > /sys/bus/pci/devices/0000\:18\:00.0/remove
echo 1 > /sys/bus/pci/devices/0000\:18\:00.1/remove
echo 1 > /sys/bus/pci/devices/0000\:17\:00.0/rescan

./test/build/test  "[sanity]" >> sanity_gen1.log

#speed gen5
setpci -s 18:00.0 cap10+30.b=5
setpci -s 17:00.0 3e.b=42
setpci -s 17:00.0 3e.b=2
sleep 10s
lspci -vvvs 18:00.0 | grep Lnk
echo 1 > /sys/bus/pci/devices/0000\:18\:00.0/remove
echo 1 > /sys/bus/pci/devices/0000\:18\:00.1/remove
echo 1 > /sys/bus/pci/devices/0000\:17\:00.0/rescan

#./install.sh
