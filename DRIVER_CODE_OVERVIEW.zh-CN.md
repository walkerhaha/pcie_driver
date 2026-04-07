# PCIe 驱动代码说明（中文）

本文档用于说明 `/driver` 目录下各部分代码分别负责什么功能，方便快速理解这个 PCIe 驱动的整体结构。

## 1. 驱动整体架构

`driver/Makefile` 会构建 4 个内核模块：

1. `mt_emu_gpu.ko`：主 GPU 驱动
2. `mt_emu_apu.ko`：APU 驱动
3. `mt_emu_vgpu.ko`：vGPU / VF 驱动
4. `mt_emu_mtdma.ko`：独立的 MTDMA 核心驱动

它们的关系可以理解为：

- `mt-emu-gpu.c` / `mt-emu-apu.c` / `mt-emu-vgpu.c` 负责 **PCIe 设备探测、BAR 映射、misc 设备注册、设备生命周期管理**
- `mt-emu-ioctl.c` 负责 **用户态入口**
- `mt-emu-intr.c` 负责 **中断初始化和回收**
- `mt-emu-mtdma-bare.c` 与 `mt-emu-mtdma-test.c` 负责 **DMA 传输与测试**
- `mt-emu-mtdma-core.c` + `virt-dma.c` 负责 **基于 dmaengine 的 DMA 核心封装**
- `mt-emu-dmabuf.c` 负责 **DMA Buffer 暴露**
- `mmu_init_pagetable.c` / `eata_api.c` 负责 **地址映射、页表和 EATA 配置**

---

## 2. 四个内核模块分别做什么

### 2.1 `mt_emu_gpu.ko`

对应文件组合：

- `mt-emu-gpu.c`
- `mt-emu-ioctl.c`
- `mt-emu-dmabuf.c`
- `mt-emu-intr.c`
- `mt-emu-mtdma-bare.c`
- `mt-emu-mtdma-test.c`
- `eata_api.c`
- `mmu_init_pagetable.c`

主要职责：

- 匹配多个 GPU PCI Device ID
- `probe` 时完成 PCI 设备使能、BAR0/BAR2/BAR4 映射、AER 使能
- 创建 `/dev/mt_emu_gpu`
- 导出 sysfs 属性：`version`、`bar0`、`bar2`、`bar4`、`bar6`、`vf`
- 初始化中断
- 初始化 DMA Common 区和 DMA bare 通道信息
- 创建 DMA buffer 设备
- 调用 `mtdma_probe()` 接入 DMA 核心
- 配置 SR-IOV/VF、IATU、EATA 等与 PF 侧相关的逻辑

从实现风格上看，它是整个仓库里功能最完整的驱动入口。

### 2.2 `mt_emu_apu.ko`

对应文件组合：

- `mt-emu-apu.c`
- `mt-emu-ioctl.c`
- `mt-emu-mtdma-bare.c`
- `mt-emu-mtdma-test.c`

主要职责：

- 匹配 APU 设备
- 完成 BAR0/BAR2 映射
- 创建 `/dev/mt_emu_apu`
- 初始化中断
- 复用 ioctl 和 DMA bare 能力

与 GPU 版本相比，APU 驱动更轻量，没有接入 `dmabuf`、`EATA`、`MMU 页表` 这些 GPU 侧附加逻辑。

### 2.3 `mt_emu_vgpu.ko`

对应文件组合：

- `mt-emu-vgpu.c`
- `mt-emu-ioctl.c`
- `mt-emu-mtdma-bare.c`
- `mt-emu-mtdma-test.c`

主要职责：

- 匹配 vGPU / VF 设备
- 完成 VF 侧 BAR 映射
- 创建 `/dev/mt_emu_vgpu`
- 使用静态次设备号：`100 + devfn`
- 初始化 VF 侧中断
- 根据 VF 的地址布局构建 DMA bare 通道
- 调用 `mtdma_probe()` 让 VF 也能走统一的 DMA 内核框架

它更像是 PF 驱动的“简化版本”，专门服务于虚拟函数。

### 2.4 `mt_emu_mtdma.ko`

对应文件组合：

- `mt-emu-mtdma-core.c`
- `virt-dma.c`

主要职责：

- 封装 Synopsys DesignWare MTDMA 控制器
- 用 Linux `dmaengine` 模型组织读写通道
- 管理 descriptor / chunk / burst
- 处理 DMA 通道启动、暂停、终止、完成回调
- 向其它模块导出 `mtdma_probe()` / `mtdma_remove()`

这个模块更偏“通用 DMA 核心层”，上面的 GPU/APU/vGPU 驱动通过它复用 DMA 能力。

---

## 3. 主入口文件说明

### 3.1 `/driver/mt-emu-gpu.c`

这是 GPU PF 驱动主文件，承担以下职责：

- 定义字符设备的 `file_operations`
- 定义 sysfs 属性展示函数
- 定义 `pcie_emu_gpu_probe()` / `pcie_emu_gpu_remove()`
- 在 `probe` 中组织整个初始化顺序
- 处理 SR-IOV 使能/关闭
- 处理部分 BAR resize / ROM / IATU 相关逻辑
- 注册 `pci_driver`

如果把整个 GPU 驱动看成一个总控器，这个文件就是总控入口。

### 3.2 `/driver/mt-emu-apu.c`

这是 APU 驱动主文件，职责与 GPU 驱动类似，但明显更精简：

- 只做 APU 需要的 BAR 映射和中断初始化
- 创建 misc 设备
- 提供 AER 错误处理回调
- 注册 PCI 驱动

### 3.3 `/driver/mt-emu-vgpu.c`

这是 vGPU 驱动主文件：

- 负责 VF 设备的探测与清理
- 注册 misc 设备
- 初始化 VF DMA 信息
- 初始化 VF 中断
- 接入 `mtdma_probe()`
- 提供 vGPU 的 AER 恢复框架

### 3.4 `/driver/mt-emu-mtdma-core.c`

这是 DMA 引擎核心实现文件：

- 抽象 `mtdma_chan` / `mtdma_desc` / `mtdma_chunk` / `mtdma_burst`
- 负责把 scatter-gather / descriptor 写成硬件可执行格式
- 负责 `issue_pending`、`terminate_all`、`pause`、`resume`
- 负责 DMA 完成与异常中断处理
- 提供 `mtdma_probe()` / `mtdma_remove()` 供其它模块调用

如果只关心“DMA 引擎是怎么跑起来的”，这个文件最关键。

---

## 4. 用户态接口相关代码

### 4.1 `/driver/mt-emu-ioctl.c`

这是用户态访问驱动的统一入口，主要实现：

- `open/read/write/release`
- `mmap`
- `ioctl`

关键点：

- `mmap` 支持把 BAR 空间映射给用户态
- `MT_IOCTL_BAR_RW`：读写 BAR 空间
- `MT_IOCTL_CFG_RW`：读写 PCI 配置空间
- `MT_IOCTL_WAIT_INT`：等待指定中断源完成
- `MT_IOCTL_IRQ_INIT`：初始化/切换中断模式
- `MT_IOCTL_MTDMA_BARE_RW`：走 bare DMA 传输
- `MT_IOCTL_MTDMA_RW`：走 dmaengine 封装 DMA
- `MT_IOCTL_DMAISR_SET`：控制 DMA ISR 行为
- `MT_IOCTL_TRIG_INT`：触发测试中断

可以把它理解成“驱动对用户程序暴露的控制面”。

### 4.2 `/driver/mt-emu-ioctl.h`

只负责声明 `mt_test_open/read/write/mmap/release/ioctl` 这些接口，供 GPU/APU/vGPU 三个入口文件复用。

---

## 5. 中断相关代码

### 5.1 `/driver/mt-emu-intr.c`

这是中断子系统实现，主要职责：

- 初始化 PF/VF 侧中断控制
- 配置中断复用器、分组和路由
- 申请 MSI/MSI-X/Legacy 中断
- 回收中断资源

从函数命名上可以看出，这里包含两类硬件侧中断初始化流程：

- `pcie_bd_*`：一类板级/中断拓扑初始化
- `pcie_cd_*`：另一类控制域中断初始化

### 5.2 `/driver/mt-emu-intr.h`

对外声明中断初始化和中断释放接口。

---

## 6. DMA 相关代码

### 6.1 `/driver/mt-emu-mtdma-bare.c`

这是“裸 DMA”实现，不依赖完整的 Linux dmaengine 提交路径，主要做：

- 初始化 DMA Common 配置
- 构建 PF/VF 使用的 DMA 通道信息
- 初始化 `dma_bare` 结构
- 直接下发 DMA 描述符链表
- 处理中断完成或轮询完成

核心函数是：

- `mtdma_comm_init()`
- `build_dma_info()`
- `build_dma_info_vf()`
- `mtdma_bare_init()`
- `mtdma_bare_init_vf()`
- `dma_bare_xfer()`

这个文件更贴近硬件寄存器和链式描述符本身。

### 6.2 `/driver/mt-emu-mtdma-bare.h`

这个头文件定义了大量 DMA 寄存器偏移、位定义和 descriptor 结构：

- DMA Common 寄存器
- 通道寄存器
- 中断状态位
- `struct dma_ch_desc`

因此它属于“裸 DMA 的寄存器协议定义文件”。

### 6.3 `/driver/mt-emu-mtdma-test.c`

这个文件更像 DMA 功能测试/封装层，主要职责：

- 初始化 `emu_mtdma`
- 封装一次 DMA 读写流程
- 提供完成回调
- 被 ioctl 路径调用，执行测试或实际搬运

### 6.4 `/driver/mt-emu-mtdma-test.h`

声明 DMA 测试层接口。

### 6.5 `/driver/mt-emu-mtdma-core.h`

定义 DMA 核心层的数据结构：

- `mtdma_chip`
- `mtdma`
- `mtdma_chan`
- `mtdma_desc`
- `mtdma_chunk`
- `mtdma_burst`

这个头文件决定了 DMA 核心层对象模型。

### 6.6 `/driver/virt-dma.c` 与 `/driver/virt-dma.h`

这是 Linux 虚拟 DMA 通道的通用实现/适配层，主要用于：

- 管理虚拟 DMA channel
- 组织待处理 descriptor 队列
- 提供 DMA 核心层可以复用的通道基础设施

`mt-emu-mtdma-core.c` 会直接依赖它。

### 6.7 `/driver/dmaengine.h`

提供 DMA 引擎接口所需的配套定义，作用是补齐 DMA 相关抽象的头文件依赖。

---

## 7. DMA Buffer 相关代码

### 7.1 `/driver/mt-emu-dmabuf.c`

这个文件负责把一块 DMA buffer 以 misc 设备形式暴露出来，主要做：

- 创建 `/dev/mt_emu_dmabuf`
- 支持 `read/write/mmap`
- 在保留内存模式下直接 `ioremap` 固定物理地址
- 在非保留模式下用 `dma_alloc_coherent()` 申请连续内存
- 导出 sysfs 的 `bar0` 信息，显示 buffer 的物理地址、虚拟地址和大小

它本质上是在给用户态提供一块“可直接配合 DMA 使用的共享缓冲区”。

### 7.2 `/driver/mt-emu-dmabuf.h`

声明 DMA buffer 设备的 probe/remove 接口。

---

## 8. 地址映射、页表与 EATA

### 8.1 `/driver/mt-emu.h`

这是非常关键的共享头文件，主要提供：

- `struct emu_pcie`：整个驱动运行期核心对象
- `struct emu_region`
- `struct emu_dmabuf`
- `struct dma_bare`
- `struct emu_mtdma`
- IATU 入站/出站配置函数
- `file_to_pcie()` / `file_to_mtdma()` 这类辅助函数

可以把它理解成“驱动全局上下文定义 + 常用内联辅助函数”。

### 8.2 `/driver/mt-emu-drv.h`

这是另一个核心配置头文件，主要负责：

- 定义设备 ID
- 定义设备名
- 定义中断源编号
- 定义 ioctl 命令号
- 定义 DDR 布局、MTDMA 缓冲区大小、VF 地址布局
- 定义一些测试参数结构体

驱动的很多运行行为，其实都被这个头文件里的宏控制。

### 8.3 `/driver/mmu_init_pagetable.c`

这个文件负责 MMU 页表初始化，主要内容包括：

- 生成虚拟地址池
- 生成物理地址池
- 生成各级页表项
- 初始化页表
- 初始化 MMU 配置

说明这个驱动不仅在做 PCIe BAR 映射，也在准备设备侧地址翻译所需的数据结构。

### 8.4 `/driver/mmu_init_pagetable.h`

定义 MMU 页表的描述符结构、页大小枚举和相关接口声明，是页表子系统的头文件入口。

### 8.5 `/driver/eata_api.c`

这个文件负责 EATA 相关初始化和描述符控制，按模块分成三类：

- GPU EATA
- Video EATA
- MTDMA EATA

主要职责：

- 初始化不同单元的 EATA 通用配置
- 初始化 TZC 相关内容
- 初始化 EATA 描述符
- 使能/关闭 descriptor 映射

它更像是“设备侧地址访问白名单 / 地址转换描述”的配置层。

### 8.6 `/driver/eata_api.h`

只做 EATA 接口声明，供 GPU 驱动和 MMU/EATA 初始化逻辑调用。

---

## 9. 内存管理相关代码

### 9.1 `/driver/mm.c`

这个文件不是 Linux 通用页分配器，而是一个独立的视频内存分配器实现，特点是：

- 以页为单位管理显存/设备内存
- 维护 free tree / alloc tree
- 使用 AVL 树做空闲块和已分配块管理

它更适合做设备本地内存或逻辑地址空间管理。

### 9.2 `/driver/mm.h`

定义了：

- `video_mm_t`
- `page_t`
- `avl_node_t`
- `vmem_init/vmem_alloc/vmem_free/vmem_get_info`

是这套视频内存分配器的公共头文件。

---

## 10. 寄存器与常量定义文件

### 10.1 `/driver/module_reg.h`

统一收拢模块会用到的寄存器头文件，是很多 `.c` 文件的公共入口。

### 10.2 `/driver/reg_define.h`

大体量寄存器宏定义文件，包含了很多硬件寄存器地址和字段定义，属于底层寄存器手册在代码里的映射。

### 10.3 `/driver/comm_define.h`

放置公共地址、布局、常量等共享定义，用于不同功能块之间统一地址语义。

### 10.4 `/driver/qy_intd.h` / `/driver/qy_intd_def.h` / `/driver/qy_plic.h`

这些文件是中断控制器相关头文件，主要用于：

- 定义中断控制器寄存器
- 定义 SPI / SGI / PLIC 等编号
- 配合 `mt-emu-intr.c` 完成中断初始化与路由

### 10.5 `/driver/insmod.h`

用于模块加载场景下的辅助定义，属于驱动配置配套头文件。

---

## 11. 代码阅读顺序建议

如果第一次接触这个仓库，建议按下面顺序看：

1. `driver/Makefile`：先看会生成哪些模块
2. `mt-emu-drv.h`：先理解设备 ID、ioctl、内存布局
3. `mt-emu.h`：再看核心数据结构
4. `mt-emu-gpu.c`：看完整的 probe/remove 主流程
5. `mt-emu-ioctl.c`：看用户态如何进入驱动
6. `mt-emu-intr.c`：看中断怎么初始化
7. `mt-emu-mtdma-bare.c`：看最贴近硬件的 DMA 路径
8. `mt-emu-mtdma-core.c` + `virt-dma.c`：看 dmaengine 封装
9. `mt-emu-dmabuf.c`：看 DMA buffer 如何给用户态使用
10. `mmu_init_pagetable.c` + `eata_api.c`：看地址翻译和访问描述配置

---

## 12. 一句话总结每部分代码的作用

- **GPU/APU/vGPU 主文件**：负责把 PCIe 设备接进 Linux 驱动框架
- **ioctl 文件**：负责给用户态发命令
- **intr 文件**：负责把硬件中断接进内核
- **mtdma-bare 文件**：负责最底层 DMA 操作
- **mtdma-core + virt-dma**：负责标准化 DMA 引擎能力
- **dmabuf 文件**：负责给 DMA 准备共享缓冲区
- **mmu/eata 文件**：负责设备侧地址映射与访问描述
- **mm 文件**：负责设备内存分配
- **各种寄存器头文件**：负责把硬件寄存器抽象成代码宏

如果后续你需要，我还可以继续补一版：

1. **按函数级别展开的说明文档**
2. **带初始化时序图的说明文档**
3. **面向用户态测试程序的 ioctl 使用说明**
