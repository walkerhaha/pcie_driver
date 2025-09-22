#echo 1 > /sys/bus/pci/devices/0000\:af\:00.0/remove
#echo 1 > /sys/bus/pci/devices/0000\:ae\:02.0/rescan

export EMU_DRV_DIR=`pwd`    
echo 8  > /proc/sys/kernel/printk
echo 0 > /proc/sys/kernel/hung_task_timeout_secs
echo 'module mt_emu_gpu +p' > /sys/kernel/debug/dynamic_debug/control
#echo 'module mt_emu_dma +p' > /sys/kernel/debug/dynamic_debug/control
#echo 1 >  /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages
#sudo modprobe virt-dma
sudo rmmod mt_emu_vgpu
#sudo rmmod mt_emu_apu
sudo rmmod mt_emu_gpu
sudo rmmod mt_emu_mtdma
sudo insmod ${EMU_DRV_DIR}/driver/mt_emu_mtdma.ko
sudo insmod ${EMU_DRV_DIR}/driver/mt_emu_gpu.ko
#sudo insmod ${EMU_DRV_DIR}/driver/mt_emu_apu.ko
sudo insmod ${EMU_DRV_DIR}/driver/mt_emu_vgpu.ko
sudo dmesg -c
cd ${EMU_DRV_DIR}/test/build
