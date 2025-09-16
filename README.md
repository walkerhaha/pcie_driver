# MT EMU PCIe driver and test app

**How to build:**  
In the driver directory:  
>> run "make"

In the test directory:  
>> mkdir build && cd build
>> cmake ..
>> run "make"

**Note:**  
Because CMA is not enalbed by default, here use reserved memory (EDK has 64G memory, 32G is used for dma, In the grub.cfg "mem=32G memmap=32G\$32G") for the DMA large buffer(no need to study the huge page in EDK)