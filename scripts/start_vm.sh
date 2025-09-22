#!/bin/sh
IMG=/home/mt/liuy/kvm.img

echo "Run QEMU"
echo 1ed5 03aa |sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
sudo qemu-system-x86_64 -D ./log.txt -cpu qemu64,+avx -boot d -hda $IMG -m 4G -smp 4 -vnc :6 -full-screen --enable-kvm -device pci-bridge,id=bridge0,chassis_nr=1 -net user,hostfwd=tcp::10023-:22 -net nic -device vfio-pci,host=0000:18:00.2
