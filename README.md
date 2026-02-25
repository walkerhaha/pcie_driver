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

---

## 代码结构分析 / Code Structure Analysis

本仓库是 Moore Threads（摩尔线程）MT 系列 GPU/APU 的 Linux PCIe 驱动程序，以及配套的用户态测试程序。

This repository contains the Linux PCIe driver for Moore Threads (MT) GPU/APU hardware, along with a userspace test application.

---

### 目录结构 / Directory Layout

```
pcie_driver/
├── driver/          # Linux 内核驱动模块 / Linux kernel driver modules
├── test/            # 用户态测试程序 / Userspace test application
└── scripts/         # 辅助脚本（安装、调试、测试）/ Helper scripts
```

---

### 驱动模块 / Driver Modules (`driver/`)

Makefile 编译产生四个内核模块 / The Makefile builds four kernel modules:

| 模块 / Module      | 源文件 / Source Files                                                                  | 说明 / Description                         |
|--------------------|----------------------------------------------------------------------------------------|--------------------------------------------|
| `mt_emu_gpu.ko`    | `mt-emu-gpu.c`, `mt-emu-ioctl.c`, `mt-emu-dmabuf.c`, `mt-emu-intr.c`, `mt-emu-mtdma-bare.c`, `mt-emu-mtdma-test.c`, `eata_api.c`, `mmu_init_pagetable.c` | GPU PF（物理功能）驱动 / GPU Physical Function driver |
| `mt_emu_apu.ko`    | `mt-emu-apu.c`, `mt-emu-ioctl.c`, `mt-emu-mtdma-bare.c`, `mt-emu-mtdma-test.c`        | APU 驱动 / APU driver                      |
| `mt_emu_vgpu.ko`   | `mt-emu-vgpu.c`, `mt-emu-ioctl.c`, `mt-emu-mtdma-bare.c`, `mt-emu-mtdma-test.c`       | 虚拟 GPU（SR-IOV VF）驱动 / Virtual GPU (SR-IOV VF) driver |
| `mt_emu_mtdma.ko`  | `mt-emu-mtdma-core.c`, `virt-dma.c`                                                    | MTDMA DMA 引擎驱动 / MTDMA DMA engine driver |

#### 关键头文件 / Key Header Files

- **`mt-emu-drv.h`** — 设备类型、PCI Vendor/Device ID、IOCTL 命令、内存地址布局、DMA 参数结构体等全局定义。  
  Global definitions: device types, PCI IDs, IOCTL commands, memory layout, DMA parameter structs.

- **`mt-emu.h`** — 核心数据结构定义，包括 `emu_pcie`（驱动主结构）、`emu_region`（BAR 映射区域）、`emu_mtdma`、`dma_bare` 等。还包含 iATU（地址转换单元）编程的内联函数。  
  Core data structures: `emu_pcie` (main driver struct), `emu_region` (BAR regions), `emu_mtdma`, `dma_bare`, and iATU programming helpers.

- **`module_reg.h`** / **`reg_define.h`** — PCIe IP 寄存器地址及位域定义。  
  PCIe IP register addresses and bit-field definitions.

- **`mt-emu-mtdma-core.h`** — MTDMA 引擎通道、描述符、chunk 等数据结构（基于 Synopsys DesignWare 修改）。  
  MTDMA engine data structures (based on Synopsys DesignWare, modified by Moore Threads).

---

### 核心数据结构 / Core Data Structures

#### `struct emu_pcie`（`mt-emu.h`）

驱动的主结构体，每个 PCI 设备对应一个实例：  
Main per-device structure, one instance per PCI device:

```c
struct emu_pcie {
    u32             type;        // GPU / APU / VGPU
    struct pci_dev  *pcid;       // 对应的 PCI 设备 / The PCI device
    struct miscdevice miscdev;   // 用户态接口 /dev/mt_emu_xxx
    struct emu_mtdma emu_mtdma;  // DMA 引擎（高层封装）/ DMA engine (high-level)
    struct dma_bare  dma_bare;   // 裸 DMA 通道 / Bare DMA channels
    struct emu_region region[7]; // BAR 区域映射 / BAR region mappings
    // 中断、互斥锁、完成量... / IRQ, mutexes, completions...
};
```

#### `struct emu_region`（`mt-emu.h`）

描述一个 PCIe BAR 的物理地址、ioremap 虚拟地址和大小：  
Describes one PCIe BAR: physical address, ioremap virtual address, and size:

```c
struct emu_region {
    phys_addr_t      paddr;
    void __iomem    *vaddr;
    resource_size_t  size;
};
```

---

### 驱动探测流程 / Driver Probe Flow

以 GPU 驱动（`mt-emu-gpu.c`）为例，`probe` 函数执行以下步骤：  
Taking the GPU driver (`mt-emu-gpu.c`) as example, the `probe` function:

1. `pci_enable_device()` — 使能 PCI 设备 / Enable PCI device  
2. `pcim_iomap_regions()` — 映射 BAR0/BAR2/BAR4 / Map BARs  
3. `pci_set_master()` — 开启总线主控（DMA 必须）/ Enable bus mastering (required for DMA)  
4. `pci_enable_pcie_error_reporting()` — 开启 AER 错误报告 / Enable AER  
5. 初始化 `emu_pcie` 结构体 / Initialize `emu_pcie` struct  
6. `irq_init()` — 申请 MSI/MSI-X 中断 / Request MSI/MSI-X interrupts  
7. `mt_emu_dmabuf_init()` — 初始化 DMA 缓冲区 / Init DMA buffer  
8. `misc_register()` — 注册 `/dev/mt_emu_gpu` 字符设备 / Register misc chardev  
9. 编程 iATU 入站/出站地址转换窗口 / Program iATU inbound/outbound ATU windows  

---

### iATU 地址转换 / iATU Address Translation (`mt-emu.h`)

iATU（Internal Address Translation Unit）将 PCIe 地址空间与本地内存地址互相映射：  
The iATU maps between PCIe address space and local memory:

- `pcie_prog_inbound_atu()` — 将主机（Host）发来的 BAR 访问映射到设备本地内存  
  Maps host BAR accesses to device-local memory  
- `pcie_prog_outbound_atu()` — 将设备本地地址映射到 PCIe 总线上的主机内存  
  Maps device-local addresses to host memory on the PCIe bus  
- `pcie_prog_inbound_atu_vf()` / `pcie_prog_inbound_atu_vf_seperate()` — VF（虚拟功能）的入站映射  
  Inbound mapping variants for SR-IOV Virtual Functions  

---

### MTDMA DMA 引擎 / MTDMA DMA Engine

MTDMA 有两层抽象：  
MTDMA has two abstraction layers:

#### 1. 裸 DMA（Bare DMA，`mt-emu-mtdma-bare.c/h`）

直接操作 DMA 寄存器，适合简单的点对点传输：  
Directly programs DMA registers, suitable for simple point-to-point transfers:

- `mtdma_comm_init()` — 初始化公共寄存器 / Init common DMA registers  
- `mtdma_bare_init()` — 初始化所有 DMA 通道 / Init all DMA channels  
- `dma_bare_xfer()` — 发起一次 DMA 传输并等待完成 / Submit and wait for a DMA transfer  
- `dma_bare_isr()` — DMA 中断服务函数 / DMA interrupt handler  

DMA 描述符结构（`struct dma_ch_desc`）包含：描述符操作码、传输字节数、源地址（SAR）、目的地址（DAR）、链表地址（LAR）。  
DMA descriptor (`struct dma_ch_desc`) contains: opcode, byte count, source address (SAR), destination address (DAR), link-list address (LAR).

#### 2. DMA 引擎（`mt-emu-mtdma-core.c/h`）

基于 Linux DMA Engine 框架，支持虚拟 DMA 通道、scatter-gather 等高级特性：  
Based on the Linux DMA Engine framework, supports virtual channels, scatter-gather, etc.:

- `struct mtdma_chip` — 控制器硬件抽象 / Controller hardware abstraction  
- `struct mtdma_chan` — 单个 DMA 通道（含虚拟通道 `virt_dma_chan`）/ Single DMA channel  
- `struct mtdma_desc` / `struct mtdma_chunk` / `struct mtdma_burst` — 传输描述符层级结构 / Transfer descriptor hierarchy  
- `mtdma_probe()` / `mtdma_remove()` — 引擎的注册/注销 / Engine registration/removal  

---

### 中断处理 / Interrupt Handling (`mt-emu-intr.c/h`)

- 支持 Legacy / MSI / MSI-X 三种中断类型（由 `irq_type` 字段控制）。  
  Supports Legacy, MSI, and MSI-X interrupt types (controlled by `irq_type`).  
- `irq_init()` — 申请 MSI 向量，为 GPU/APU 注册中断处理函数 / Request MSI vectors and register ISR  
- `qy_free_irq()` — 释放中断 / Release IRQs  
- 使用 `completion` 机制让用户态 ioctl 等待特定中断事件（最多 32 个中断源，见 `QY_INT_SRC_*` 枚举）。  
  Uses `completion` to let userspace ioctl wait for specific IRQ events (up to 32 sources, see `QY_INT_SRC_*` enum).  
- 支持 PLIC（Platform-Level Interrupt Controller）配置，见 `qy_plic.h`。  
  Supports PLIC (Platform-Level Interrupt Controller) configuration, see `qy_plic.h`.

---

### 用户态接口 / Userspace Interface (`mt-emu-ioctl.c/h`)

通过 `/dev/mt_emu_gpu`（或 `mt_emu_apu`、`mt_emu_vgpu`）提供以下接口：  
Exposed via `/dev/mt_emu_gpu` (or `mt_emu_apu`, `mt_emu_vgpu`):

| 接口 / Interface    | 说明 / Description                                              |
|--------------------|-----------------------------------------------------------------|
| `open` / `release` | 设备打开/关闭 / Device open/close                               |
| `read` / `write`   | BAR 区域数据读写 / BAR region data read/write                   |
| `mmap`             | 将 BAR 或 DMA 缓冲区映射到用户地址空间 / Map BAR/DMA buf to userspace |
| `ioctl`            | 见下表 / See table below                                        |

**IOCTL 命令 / IOCTL Commands**（定义于 `mt-emu-drv.h`）：

| 命令 / Command            | 功能 / Function                                        |
|--------------------------|--------------------------------------------------------|
| `MT_IOCTL_BAR_RW`        | 读写 BAR 寄存器 / Read/write BAR registers             |
| `MT_IOCTL_CFG_RW`        | 读写 PCIe 配置空间 / Read/write PCIe config space      |
| `MT_IOCTL_READ_ROM`      | 读取 ROM / Read ROM                                    |
| `MT_IOCTL_WAIT_INT`      | 等待中断 / Wait for interrupt                          |
| `MT_IOCTL_IPC`           | 进程间通信（Mailbox）/ IPC via Mailbox                 |
| `MT_IOCTL_IRQ_INIT`      | 初始化中断类型 / Init IRQ type                         |
| `MT_IOCTL_MTDMA_BARE_RW` | 裸 DMA 读写 / Bare DMA read/write                      |
| `MT_IOCTL_MTDMA_RW`      | DMA 引擎读写 / DMA engine read/write                   |
| `MT_IOCTL_DMAISR_SET`    | 设置 DMA 中断模式 / Set DMA ISR mode                   |
| `MT_IOCTL_TRIG_INT`      | 触发中断（测试用）/ Trigger interrupt (for testing)    |

---

### SR-IOV 虚拟功能 / SR-IOV Virtual Functions (`mt-emu-vgpu.c`)

- GPU PF 驱动支持最多 60 个 VF（`VF_NUM=60`）。  
  The GPU PF driver supports up to 60 Virtual Functions.  
- 每个 VF 分配独立的 DDR 区域（`SIZE_VGPU_DDR = 1GB`）和 MTDMA 链表内存。  
  Each VF gets its own DDR region (1 GB) and MTDMA link-list memory.  
- 通过 iATU VF 入站映射将 VF BAR 访问路由到各自的内存区域。  
  VF BAR accesses are routed to per-VF memory regions via iATU VF inbound mappings.

---

### 内存地址布局 / Memory Address Layout (`mt-emu-drv.h`)

设备 DDR（默认 48GB）的布局如下 / Device DDR (default 48 GB) layout:

```
地址 / Address          大小 / Size      用途 / Usage
0x0000_0000_0000      DDR_SZ_FREE      GPU / 通用测试内存 / GPU & test memory
DDR_SZ_FREE           32 MB            APU 内存 / APU memory
+32 MB                64 KB            MTDMA 写通道链表 / MTDMA WR channel link-list
+64 KB                64 KB            MTDMA 读通道链表 / MTDMA RD channel link-list
...                   ...              ...
DDR_SZ_FREE - VF*1GB  VF_NUM * 1GB     VF GPU 内存区域 / VF GPU memory regions
```

出站窗口 / Outbound window:
```
LADDR_OUTBOUND = 0x80_0000_0000 (512 GB 偏移)，映射到主机内存
```

---

### 测试程序 / Test Application (`test/`)

用户态测试程序基于 CMake 构建，使用 Catch2 测试框架：  
The userspace test app is CMake-based and uses the Catch2 framework:

- **`src/main.cc`** — 测试入口，初始化 PCIe 功能库，运行 Catch2 测试套件  
- **`src/intr.cc`** — 中断测试用例 / Interrupt test cases  
- **`src/mthdma.cc`** — MTDMA 传输测试 / MTDMA transfer tests  
- **`src/stress.cc`** — 压力测试 / Stress tests  
- **`src/base.cc`** — 基础寄存器读写测试 / Basic register R/W tests  
- **`lib/`** — PCIe 操作库（封装 ioctl 调用）/ PCIe operation library (wraps ioctl calls)  
- **`shell/`** — 基于 letter-shell 的交互式命令行界面 / Interactive shell based on letter-shell  

---

### 脚本说明 / Scripts (`scripts/`)

| 脚本 / Script          | 功能 / Function                                              |
|------------------------|--------------------------------------------------------------|
| `install.sh`           | 加载内核模块 / Load kernel modules                           |
| `bulid.sh`             | 构建驱动 / Build the driver                                  |
| `clean.sh`             | 清理编译产物 / Clean build artifacts                         |
| `test_sanity.sh`       | 基本功能测试 / Sanity tests                                  |
| `test_stress.sh`       | 压力测试 / Stress tests                                      |
| `FLR.sh`               | 触发 PCIe Function Level Reset / Trigger PCIe FLR            |
| `hot_reset`            | 热复位 / Hot reset                                           |
| `rom_bar_handle.sh`    | ROM BAR 操作 / ROM BAR handling                              |
| `disable_watchdog.sh`  | 关闭看门狗 / Disable watchdog                                |
| `dump_desc.c`          | 转储 DMA 描述符（调试工具）/ Dump DMA descriptors (debug)    |
| `mtdma_reg_dump_pf0.c` | 转储 MTDMA PF0 寄存器 / Dump MTDMA PF0 registers            |
| `perf_data.py`         | 解析性能数据 / Parse performance data                        |