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

## 13. 按函数级别补充说明

这一节不追求把每个静态辅助函数都展开，而是优先解释“真正决定驱动行为”的关键函数。

### 13.1 `mt-emu-gpu.c` 关键函数

| 函数 | 作用 |
|---|---|
| `pcie_emu_gpu_probe()` | GPU PF 的总初始化入口。完成设备使能、BAR 映射、AER、`emu_pcie` 分配、IRQ 初始化、misc 注册、DMA Common 初始化、dmabuf 初始化、DMA 通道组织以及 SR-IOV 使能。 |
| `pcie_emu_gpu_remove()` | GPU PF 卸载入口。先关闭 SR-IOV，再统一释放中断、misc、BAR 映射和其他附属资源。 |
| `pcie_emu_gpu_free()` | GPU 清理核心函数，真正执行 IRQ 释放、dmabuf remove、misc 注销、BAR unmap、ROM/PCI 关闭。 |
| `mt_emu_vf_enable()` | 控制 PF 侧 SR-IOV VF 的启停，并把 `vf_num` 写回到运行时上下文。 |
| `show_bar0/show_bar2/show_bar4/show_bar6/show_vf()` | 给 `/sys/class/misc/mt_emu_gpu/` 导出可读属性，让用户态知道 BAR 地址和 VF 数量。 |
| `resize_pcie_bar()` / `resize_fb_bar()` | 预留的 BAR resize 能力，主要用于尝试调整 BAR2/显存 BAR 大小，当前代码里大多以注释形式保留。 |

可以把 `pcie_emu_gpu_probe()` 看成整个 GPU 驱动的“装配总流程”。

### 13.2 `mt-emu-apu.c` 关键函数

| 函数 | 作用 |
|---|---|
| `pcie_apu_probe()` | APU 初始化入口。完成 PCI 使能、BAR0/BAR2 映射、AER、`emu_pcie` 初始化、中断初始化和 misc 注册。 |
| `pcie_apu_remove()` | APU 卸载入口。调用统一释放函数回收资源。 |
| `pcie_qy_free()` | APU 资源回收核心函数。 |
| `emu_apu_error_detected()` / `emu_apu_slot_reset()` / `emu_apu_error_resume()` | AER 错误恢复回调，用来处理 PCIe 错误检测、slot reset 后恢复和 resume。 |

APU 入口比 GPU 简单很多，当前 `probe()` 里没有像 GPU 那样继续接 `dmabuf`、EATA、MMU 初始化链路。

### 13.3 `mt-emu-vgpu.c` 关键函数

| 函数 | 作用 |
|---|---|
| `pcie_emu_vgpu_probe()` | vGPU/VF 的初始化入口。完成设备使能、BAR 映射、`emu_pcie` 建立、MSI-X 初始化、静态 minor 注册、VF DMA 通道构建、DMA bare 初始化、`emu_mtdma_init()` 和 `mtdma_probe()`。 |
| `pcie_emu_vgpu_remove()` | vGPU 卸载入口。 |
| `pcie_emu_vgpu_free()` | vGPU 资源回收核心函数。 |
| `emu_vgpu_error_detected()` / `emu_vgpu_slot_reset()` / `emu_vgpu_error_resume()` | vGPU AER 恢复路径。 |

这里最值得注意的是：vGPU 设备名会带上 `devfn`，因此用户态看到的节点会是 `/dev/mt_emu_vgpu<devfn>`。

### 13.4 `mt-emu-ioctl.c` 关键函数

| 函数 | 作用 |
|---|---|
| `mt_test_open()` | 打开设备，目前基本是空实现。 |
| `mt_test_read()` | 当前返回 0，主要接口并不走这里。 |
| `mt_test_write()` | 当前返回 0，主要接口并不走这里。 |
| `mt_test_release()` | 关闭设备，目前也是空实现。 |
| `mt_test_mmap()` | 允许用户态按 `vm_pgoff` 选择 BAR0/BAR2/BAR4/BAR6 映射到用户空间。 |
| `mt_test_ioctl()` | 最核心的用户态控制入口。几乎所有高级操作都从这里分派。 |
| `emu_dma_isr()` | 把 DMA 中断结果进一步分发给 bare DMA 或 mtdma 逻辑。 |

`mt_test_ioctl()` 里主要分支的职责如下：

| ioctl 分支 | 作用 |
|---|---|
| `MT_IOCTL_BAR_RW` | 通过 ioctl 方式读写 BAR 空间。 |
| `MT_IOCTL_CFG_RW` | 读写 PCI 配置空间。 |
| `MT_IOCTL_GET_POWER` | 查询当前 PCI 电源状态。 |
| `MT_IOCTL_SUSPEND` / `MT_IOCTL_RESUME` | 让设备进入指定电源态，或恢复到 `PCI_D0`。 |
| `MT_IOCTL_WAIT_INT` | 等待指定中断源完成。 |
| `MT_IOCTL_TRIG_INT` | 触发软中断，用于测试中断链路。 |
| `MT_IOCTL_IPC` | 给 DSP/FEC/SMC 发命令，并等待响应中断。 |
| `MT_IOCTL_IRQ_INIT` | 切换中断模式并重新初始化中断。 |
| `MT_IOCTL_READ_ROM` | 映射并读取扩展 ROM 的准备逻辑。 |
| `MT_IOCTL_MTDMA_BARE_RW` | 走 bare DMA 通道。 |
| `MT_IOCTL_MTDMA_RW` | 走 dmaengine 封装的 MTDMA 通道。 |
| `MT_IOCTL_DMAISR_SET` | 控制当前 DMA 中断按 bare 模式还是 mtdma 模式处理。 |

额外注意两点：

1. `mt_test_read()` / `mt_test_write()` 目前几乎没有实际功能，真正有用的是 `mmap + ioctl`。
2. 当前代码里 `MT_IOCTL_CFG_RW` 分支后缺少 `break`，会继续落入 `MT_IOCTL_SUSPEND` 分支。这属于一个已知实现问题，可参考 `driver/mt-emu-ioctl.c` 中 `mt_test_ioctl()` 的对应分支。

### 13.5 `mt-emu-mtdma-bare.c` 关键函数

| 函数 | 作用 |
|---|---|
| `mtdma_comm_init()` | 初始化 DMA Common 区域的通道数、burst 长度、mask 等公共寄存器。 |
| `build_dma_info()` | 为 PF 构建 DMA 读写通道的寄存器基址、链表地址、链表大小等信息。 |
| `build_dma_info_vf()` | 为 VF/vGPU 构建专用 DMA 通道布局。 |
| `mtdma_bare_init()` | 用 `mtdma_info` 初始化 PF bare DMA 运行时结构。 |
| `mtdma_bare_init_vf()` | 用 `mtdma_info` 初始化 VF bare DMA 运行时结构。 |
| `dma_bare_isr()` | 解析 bare DMA 通道完成/错误状态。 |
| `dma_bare_xfer()` | 真正下发链表描述符并启动一次 bare DMA 传输。 |

这部分是最接近硬件寄存器的 DMA 路径。

### 13.6 `mt-emu-mtdma-test.c` 关键函数

| 函数 | 作用 |
|---|---|
| `emu_mtdma_init()` | 初始化 `emu_mtdma`，建立 mtdma 芯片对象、通道、完成量等上下文。 |
| `emu_dma_rw()` | 处理一次用户态发起的 mtdma 读写请求，是 `MT_IOCTL_MTDMA_RW` 的主要后端。 |
| `mtdma_xfer()` | 单次传输的内部封装，组织 DMA channel、sg 表和回调。 |
| `mtdma_test_callback()` | DMA 完成回调。 |
| `emu_mtdma_isr()` | mtdma 模式下的中断入口。 |

### 13.7 `mt-emu-dmabuf.c` 关键函数

| 函数 | 作用 |
|---|---|
| `emu_dmabuf_probe()` | 分配或映射 DMA buffer，并注册 `/dev/mt_emu_dmabuf`。 |
| `emu_dmabuf_remove()` | 释放 DMA buffer 设备资源。 |
| `emu_dmabuf_read()` / `emu_dmabuf_write()` | 允许用户态直接读写这块 DMA buffer。 |
| `emu_dmabuf_mmap()` | 把 DMA buffer 映射给用户态。 |

### 13.8 `mmu_init_pagetable.c` 与 `eata_api.c` 关键函数

| 函数 | 作用 |
|---|---|
| `gen_va_pool()` | 生成虚拟地址池。 |
| `gen_pa_pool()` | 生成物理地址池。 |
| `gen_page_desc()` | 生成页表项。 |
| `setup_page_desc()` | 批量组织页描述符。 |
| `mmu_init_pagetable()` | 初始化整个页表布局。 |
| `pcie_mmu_cfg_init()` | 初始化 MMU 上下文配置。 |
| `pcie_mmu_init()` | 把页表初始化和寄存器初始化串起来。 |
| `gpu_eata_init()` / `vid_eata_init()` / `mtdma_eata_init()` | 初始化不同功能块的 EATA 通用配置。 |
| `mtdma_eata_desc_init()` | 初始化 MTDMA 的 EATA 描述符。 |
| `mtdma_eata_desc_en()` / `mtdma_eata_desc_dis()` | 使能/关闭某段地址映射。 |

---

## 14. 驱动初始化流程图 / 时序说明

这一节给出更偏“读代码路径”的初始化顺序。

### 14.1 GPU PF 初始化主流程

```text
PCIe 枚举到 GPU 设备
    ↓
pcie_emu_gpu_probe()
    ↓
pcim_enable_device()
    ↓
pcim_iomap_regions(BAR0/BAR2/BAR4)
    ↓
pci_enable_pcie_error_reporting()
    ↓
pci_set_master()
    ↓
devm_kzalloc(struct emu_pcie)
    ↓
初始化 mutex / completion / region[]
    ↓
pci_set_drvdata()
    ↓
irq_init(IRQ_MSI, 0)
    ↓
misc_register(/dev/mt_emu_gpu)
    ↓
mtdma_comm_init()
    ↓
emu_dmabuf_probe() -> 注册 /dev/mt_emu_dmabuf
    ↓
build_dma_info()
    ↓
mtdma_bare_init()
    ↓
emu_mtdma_init()
    ↓
mt_emu_vf_enable(VF_NUM)   [当 VF_NUM > 0]
    ↓
Probe success
```

补充理解：

- **前半段**是标准 PCIe 驱动初始化；
- **中段**开始挂 misc 设备和中断；
- **后半段**把 DMA 运行环境组织起来；
- **最后**才决定是否打开 SR-IOV。

### 14.2 APU 初始化主流程

```text
PCIe 枚举到 APU 设备
    ↓
pcie_apu_probe()
    ↓
pci_enable_device()
    ↓
pcim_iomap_regions(BAR0/BAR2)
    ↓
pci_set_master()
    ↓
pci_enable_pcie_error_reporting()
    ↓
devm_kzalloc(struct emu_pcie)
    ↓
初始化 mutex / completion / region[]
    ↓
irq_init(IRQ_MSI, 0)
    ↓
misc_register(/dev/mt_emu_apu)
    ↓
Probe success
```

APU 的路径明显比 GPU 短，当前代码没有继续串接 `dmabuf -> bare DMA -> emu_mtdma_init` 这一整套初始化流程。

### 14.3 vGPU / VF 初始化主流程

```text
PCIe 枚举到 vGPU 设备
    ↓
pcie_emu_vgpu_probe()
    ↓
pcim_enable_device()
    ↓
pcim_iomap_regions(BAR0/BAR2)
    ↓
pci_set_master()
    ↓
devm_kzalloc(struct emu_pcie)
    ↓
初始化 mutex / spinlock / completion / region[]
    ↓
irq_init(IRQ_MSIX, 0)
    ↓
misc_register(/dev/mt_emu_vgpu<devfn>)
    ↓
build_dma_info_vf()
    ↓
mtdma_bare_init_vf()
    ↓
emu_mtdma_init()
    ↓
mtdma_probe()
    ↓
Probe success
```

vGPU 与 GPU 最大不同点：

- 不做 PF 那套 `dmabuf + SR-IOV` 逻辑；
- 直接按 VF 地址布局构建 DMA 通道；
- misc 设备名和 minor 号都与 `devfn` 绑定。

### 14.4 用户态发起一次 bare DMA 的时序

```text
用户程序
    ↓
pcief_dma_bare_xfer()                  [test/lib/mt_pcie_f.c]
    ↓
ioctl(fd, MT_IOCTL_DMAISR_SET, ...)
    ↓
ioctl(fd, MT_IOCTL_MTDMA_BARE_RW, ...)
    ↓
mt_test_ioctl()
    ↓
dma_bare_xfer()
    ↓
写 DMA 描述符 / 启动通道
    ↓
硬件完成并触发中断
    ↓
emu_dma_isr() / dma_bare_isr()
    ↓
completion 唤醒
    ↓
ioctl 返回用户态
```

### 14.5 用户态发起一次 mtdma engine 传输的时序

```text
用户程序
    ↓
pcief_mtdma_engine_start()            [test/lib/mt_pcie_f.c]
    ↓
ioctl(fd, MT_IOCTL_DMAISR_SET, 0)
    ↓
ioctl(fd, MT_IOCTL_MTDMA_RW, ...)
    ↓
mt_test_ioctl()
    ↓
emu_dma_rw()
    ↓
mtdma_xfer()
    ↓
dmaengine / virt-dma / mtdma core
    ↓
回调 mtdma_test_callback()
    ↓
返回用户态
```

---

## 15. 面向用户态测试程序的 ioctl 使用说明

这一节对应仓库里的测试封装：`/test/lib/mt_pcie_f.c`。

### 15.1 用户态是怎么找到设备的

测试库并不是直接硬编码 BAR 地址，而是按下面流程工作：

1. 打开设备节点
   - GPU：`/dev/mt_emu_gpu`
   - DMA Buffer：`/dev/mt_emu_dmabuf`
   - vGPU：`/dev/mt_emu_vgpu<N>`
2. 读取 sysfs 属性
   - `/sys/class/misc/<device>/bar0`
   - `/sys/class/misc/<device>/bar2`
   - 以及 GPU 的 `/sys/class/misc/mt_emu_gpu/vf`
3. 根据读到的 `paddr:vaddr:size` 信息，对 BAR 做 `mmap`

也就是说，测试程序是通过 **misc 设备 + sysfs 属性** 来完成设备发现的。

### 15.2 测试库里常用封装与对应 ioctl

| 用户态封装 | 对应驱动能力 | 说明 |
|---|---|---|
| `pcief_io_write()` / `pcief_io_read()` | `MT_IOCTL_BAR_RW` | 通过 ioctl 访问 BAR 空间。 |
| `pcief_cfg_write()` / `pcief_cfg_read()` | `MT_IOCTL_CFG_RW` | 读写 PCI 配置空间。 |
| `pcief_get_power()` | `MT_IOCTL_GET_POWER` | 获取 PCI 电源状态。 |
| `pcief_suspend()` | `MT_IOCTL_SUSPEND` | 请求设备切到指定电源态。 |
| `pcief_resume()` | `MT_IOCTL_RESUME` | 让设备恢复到 `PCI_D0`。 |
| `pcief_wait_int()` | `MT_IOCTL_WAIT_INT` | 等待指定中断源。 |
| `pcief_trig_int()` | `MT_IOCTL_TRIG_INT` | 触发软中断测试。 |
| `pcief_irq_init()` | `MT_IOCTL_IRQ_INIT` | 初始化或切换中断模式。 |
| `pcief_tgt_cmd()` | `MT_IOCTL_IPC` | 向 DSP/FEC/SMC 发命令并等待返回。 |
| `pcief_dma_bare_xfer()` | `MT_IOCTL_DMAISR_SET` + `MT_IOCTL_MTDMA_BARE_RW` | 走 bare DMA。 |
| `pcief_mtdma_engine_start()` | `MT_IOCTL_DMAISR_SET` + `MT_IOCTL_MTDMA_RW` | 走 dmaengine/mtdma 封装路径。 |

### 15.3 `mt_emu_param` 的意义

大多数 ioctl 都把用户缓冲区的起始位置解释成一个 `struct mt_emu_param`：

- `b0/b1/b2/b3`：常放小字段，例如 `bar`、`rw`、`irq type`
- `d0/d1`：常放长度、状态、超时值
- `l0/l1`：常放 offset、地址

典型例子：

- `MT_IOCTL_BAR_RW`
  - `b0 = bar`
  - `b1 = BAR_RD / BAR_WR`
  - `d0 = size`
  - `l0 = offset`
  - 结构体后面紧跟实际读写数据

- `MT_IOCTL_WAIT_INT`
  - `b0 = 中断源编号`
  - `d0 = timeout_ms`
  - 返回时 `b0 = 0/1` 表示是否等到

- `MT_IOCTL_IRQ_INIT`
  - `b0 = 中断类型`
  - `b1 = test_mode`
  - 返回时 `b0 = 0/1` 表示成功或失败

### 15.4 bare DMA 在用户态怎么调用

测试库调用顺序如下：

1. 构造 `struct dma_bare_rw`
2. 把它放到 `mt_emu_param` 后面
3. 先调用 `MT_IOCTL_DMAISR_SET` 让驱动按 bare DMA 模式处理中断
4. 再调用 `MT_IOCTL_MTDMA_BARE_RW`

`struct dma_bare_rw` 里的关键字段：

- `sar`：源地址
- `dar`：目的地址
- `data_direction`：方向（H2H / H2D / D2H / D2D）
- `desc_direction`：描述符放在 host 还是 device
- `desc_cnt` / `block_cnt`：链表规模
- `size`：数据量
- `ch_num`：通道号
- `timeout_ms`：超时

### 15.5 mtdma engine 在用户态怎么调用

测试库调用顺序如下：

1. 通过 `pcief_mtdma_engine_malloc()` 分配一块带对齐余量的用户缓冲区
2. 在缓冲区头部摆放 `mt_emu_param + struct mtdma_rw`
3. 若有 payload，则放在 `arg + MTDMA_BUF_START` 后面
4. 调用 `MT_IOCTL_DMAISR_SET(0)`，切换到 mtdma engine 模式
5. 调用 `MT_IOCTL_MTDMA_RW`

`struct mtdma_rw` 里的关键字段：

- `laddr`：设备侧逻辑地址
- `size`：数据大小
- `timeout_ms`：超时
- `test_cnt`：测试次数
- `ch`：通道号
- `dir`：读/写方向

### 15.6 DMA buffer 设备的用法

`/dev/mt_emu_dmabuf` 不主要靠 ioctl，而是直接用：

- `read`
- `write`
- `mmap`

测试库里的对应封装：

- `pcief_dmabuf_write()`
- `pcief_dmabuf_read()`
- `pcief_dmabuf_malloc()`
- `pcief_dmabuf_free()`

其中：

- `pcief_dmabuf_write/read()` 是对字符设备直接做 `lseek + read/write`
- `pcief_dmabuf_malloc/free()` 是测试库自己维护的用户态块分配器，不是驱动里的内核分配接口

### 15.7 当前实现下的几个注意事项

1. `MT_IOCTL_BAR_RW` 在驱动里只允许 `bar=0` 或 `bar=2`。
2. `MT_IOCTL_CFG_RW` 的已知实现问题见上文 13.4 节：当前代码会落入 `MT_IOCTL_SUSPEND` 分支。
3. `MT_IOCTL_READ_ROM` 当前代码主要完成 ROM 映射/解除映射。真正把内容复制回用户态的逻辑被注释掉了。
4. 对多数测试场景来说，**直接 mmap BAR + 少量 ioctl 控制** 才是主路径，而不是 `read/write`。

---

## 16. 当前驱动是如何把内核态和用户态隔离开的

结合 `driver/` 和 `test/lib/mt_pcie_f.c` 的实现，当前仓库里的“隔离”更准确地说是 **Linux 设备文件边界上的职责分层**，而不是严格的安全封装。

### 16.1 内核态当前负责什么

内核态主要负责硬件资源与生命周期：

- PCIe 设备探测、`probe/remove`
- BAR 资源申请与内核映射
- 中断初始化、回收和错误恢复
- DMA buffer 分配或保留内存映射
- bare DMA / mtdma engine 初始化
- SR-IOV、VF、IATU、MMU、EATA 等硬件级配置
- 注册 `/dev/mt_emu_gpu`、`/dev/mt_emu_dmabuf`、`/dev/mt_emu_vgpu<N>`
- 导出 `/sys/class/misc/<device>/bar*`、`version`、`vf` 等 sysfs 元数据

也就是说，**设备所有权、物理资源和执行路径仍然掌握在内核里**。

### 16.2 用户态当前负责什么

用户态主要负责控制、测试和策略：

- 打开 `/dev/mt_emu_*`
- 读取 `/sys/class/misc/<device>/bar*`
- 对 BAR 或 DMA buffer 做 `mmap`
- 通过 `ioctl` 发起 BAR/CFG 读写、DMA、IRQ、IPC、功耗控制
- 自己组织测试流程、参数封装、超时和结果校验

当前仓库里的测试程序基本就是按这种模式工作的，因此更接近“**用户态直接操控硬件能力，内核态提供通道**”。

### 16.3 当前边界是通过哪些接口建立的

主要只有三类接口：

1. **misc 设备节点**
   - `/dev/mt_emu_gpu`
   - `/dev/mt_emu_dmabuf`
   - `/dev/mt_emu_vgpu<N>`
2. **sysfs 元数据**
   - `bar0/bar2/bar4/bar6`
   - `version`
   - `vf`
3. **`ioctl + mmap`**
   - `copy_from_user/copy_to_user` 交换控制参数
   - `remap_pfn_range()` 把 BAR 或 DMA buffer 映射给用户态

因此当前实现的隔离点主要在于：

- 用户态不能直接拿到内核里的 `struct emu_pcie`
- 用户态不能自己接管 PCI 资源生命周期
- 中断、DMA 初始化、SR-IOV、IATU、MMU、EATA 仍由内核统一管理

### 16.4 当前边界为什么说“隔离不强”

虽然边界存在，但暴露面也很大：

- `mt_test_mmap()` 允许直接映射 BAR
- `MT_IOCTL_BAR_RW` 允许用户态按 offset 访问 BAR
- `MT_IOCTL_CFG_RW` 允许用户态访问 PCI 配置空间
- `/dev/mt_emu_dmabuf` 直接暴露一块可被 DMA 使用的共享缓冲区

这意味着当前驱动更适合：

- bring-up
- 板卡验证
- 内部调试
- 功能测试平台

如果目标是正式产品驱动，这种接口设计通常还需要进一步收口。

---

## 17. 面向重构的新驱动分层建议

如果要基于这个仓库重新构建一套更稳的驱动，建议把目标架构明确为：

- **内核态负责资源、安全边界和执行**
- **用户态负责策略、任务组织和业务逻辑**

可以理解成“**薄内核 + 厚用户态库**”，但前提是只暴露受控 ABI，而不是把整块 BAR 和任意寄存器访问直接交给用户态。

### 17.1 建议拆成四层

#### 第一层：PCIe 内核核心层

只负责硬件生命周期与平台能力：

- `pci_driver` probe/remove
- BAR 映射
- DMA mask / IOMMU
- 中断申请与回收
- reset / AER / 电源管理
- PF/VF 管理

这一层建议只面向内核内部模块，不直接暴露给用户态。

#### 第二层：内核资源服务层

把硬件能力收敛成受控服务：

- buffer 管理
- DMA 提交与完成
- doorbell / queue / mailbox
- 中断事件分发
- 上下文管理
- 引用计数与并发保护
- 多进程 / 多 VF 隔离

这一层是未来正式驱动最关键的抽象层。

#### 第三层：稳定用户态 ABI 层

这一层才是对用户态公开的正式协议：

- `ioctl`：创建上下文、提交任务、查询状态、释放资源
- `mmap`：只映射共享 ring、命令队列、受控 buffer
- `poll/eventfd`：等待完成事件
- `sysfs/debugfs`：区分正式元数据与调试能力

建议这里遵循三个约束：

1. 默认**不映射整块 BAR**
2. 默认**不开放任意 offset BAR/CFG 访问**
3. 测试接口与正式 ABI **分离**

#### 第四层：用户态运行库层

建议在用户态补一层 `libdriver`：

- 封装 `open/ioctl/mmap/poll`
- 做参数组包和错误码处理
- 统一超时、同步、日志和重试逻辑
- 管理 buffer、queue、fence
- 向上提供稳定 API

这样未来协议演进时，业务代码不用直接绑定底层驱动细节。

### 17.2 建议的模块边界清单

如果继续在当前仓库演进，建议按职责把能力收拢成下面几个模块边界：

| 建议模块 | 主要职责 | 当前代码可参考位置 |
|---|---|---|
| `core/` | PCIe probe/remove、BAR、AER、电源管理、reset | `mt-emu-gpu.c`、`mt-emu-apu.c`、`mt-emu-vgpu.c` |
| `irq/` | MSI/MSI-X/Legacy、中断路由、完成通知 | `mt-emu-intr.c` |
| `mem/` | DMA buffer、IOMMU、地址映射、页表、EATA | `mt-emu-dmabuf.c`、`mmu_init_pagetable.c`、`eata_api.c` |
| `dma/` | bare DMA 与 dmaengine 封装 | `mt-emu-mtdma-bare.c`、`mt-emu-mtdma-test.c`、`mt-emu-mtdma-core.c` |
| `abi/` | 稳定 ioctl 协议、uAPI 结构体、版本兼容 | 当前可从 `mt-emu-drv.h`、`mt-emu-ioctl.c` 中拆出 |
| `pf/` | PF 专有能力，如 SR-IOV、VF 编排 | `mt-emu-gpu.c` |
| `vf/` | VF 专有初始化和受限执行路径 | `mt-emu-vgpu.c` |
| `debug/` | debugfs、测试 ioctl、寄存器调试通道 | 当前 `MT_IOCTL_BAR_RW`、`MT_IOCTL_CFG_RW`、测试中断等能力 |
| `lib/` | 用户态封装库 | 当前 `test/lib/mt_pcie_f.c` 可作为原型参考 |
| `tools/` | 测试程序、验证工具、带外调试工具 | `test/src/`、`test/shell/` |

这里最关键的变化不是目录名本身，而是 **把“正式 ABI”与“调试/测试接口”明确分层**。

### 17.3 从当前实现迁移时最值得优先收口的能力

如果要做产品化重构，优先建议收口下面几类接口：

1. **BAR 全量 mmap**
   - 改成只映射共享队列、门铃页或受控共享缓冲区
2. **任意 BAR/CFG 读写 ioctl**
   - 仅保留 debug 通道，默认不对正式用户态开放
3. **测试 ioctl 与正式 ABI 混合**
   - 把测试路径挪到 `debugfs` 或专用调试设备
4. **PF/VF 与业务控制混杂**
   - PF 管理面、VF 执行面、测试接口分开

---

## 18. 推荐的重建顺序

结合当前仓库的实现基础，建议按下面顺序推进：

### 18.1 第一阶段：先稳定内核核心层

先做好：

- PCIe core
- probe/remove
- BAR 管理
- IRQ
- reset / AER / 电源管理

这一阶段的目标是先让设备生命周期稳定下来。

### 18.2 第二阶段：再稳定资源服务层

再收敛：

- DMA buffer 生命周期
- DMA 提交模型
- 中断完成路径
- queue / doorbell / mailbox
- 上下文与同步模型

这一阶段结束后，内核内部应当已经具备“受控执行能力”。

### 18.3 第三阶段：定义正式 uAPI

这时再定义：

- 上下文创建/销毁
- buffer 导入/导出
- 命令提交
- 完成通知
- 查询状态

也就是先把内核内部抽象稳定，再冻结用户态协议。

### 18.4 第四阶段：补用户态运行库

用户态库负责：

- ABI 封装
- 参数校验
- 同步等待
- 错误码转换
- 兼容不同驱动版本

这样上层业务就不会直接依赖底层 ioctl 细节。

### 18.5 第五阶段：最后补调试和测试能力

最后再把：

- 调试寄存器访问
- 测试 DMA
- 性能验证
- 诊断工具

作为调试层能力补齐，而不是让它们反向决定正式 ABI 形态。

---

## 19. 一句话总结当前仓库与推荐重构方向

当前仓库的模式是：

> **内核拿住设备资源，用户态通过 `/dev + sysfs + ioctl + mmap` 直接操控硬件。**

如果面向正式产品重构，更推荐的方向是：

> **内核负责资源、安全边界和执行；用户态负责策略和业务；BAR 访问尽量收敛为受控 ABI，而不是直接裸露。**

---

## 20. 当前驱动是如何拿到 device 可用地址空间的

这个问题放到当前仓库里，建议拆成两层理解：

1. **主机侧可访问地址空间 (host-visible)**
2. **device 侧逻辑地址空间 (device-local / logical)**

二者都被代码大量使用，但来源完全不同。

### 20.1 主机侧可访问地址空间：来自 PCI BAR

GPU PF 和 vGPU/VF 的 `probe()` 流程基本一致，都是先让 Linux PCI 子系统把设备资源准备好，再把 BAR 信息记到驱动自己的 `region[]` 里。

#### GPU PF 路径

`driver/mt-emu-gpu.c`

```text
pcie_emu_gpu_probe()
    ↓
pcim_enable_device()
    ↓
pcim_iomap_regions(BAR0/BAR2/BAR4)
    ↓
for each BAR:
    pci_resource_start()
    pci_resource_len()
    pcim_iomap_table()[i]
    ↓
emu_pcie->region[i]
```

对应代码位置：

- `pcim_enable_device()`：`driver/mt-emu-gpu.c:431-436`
- `pcim_iomap_regions()`：`driver/mt-emu-gpu.c:444-449`
- `pci_resource_start()/pci_resource_len()/pcim_iomap_table()`：`driver/mt-emu-gpu.c:501-513`

#### vGPU / VF 路径

`driver/mt-emu-vgpu.c`

```text
pcie_emu_vgpu_probe()
    ↓
pcim_enable_device()
    ↓
pcim_iomap_regions(BAR0/BAR2)
    ↓
for each BAR:
    pci_resource_start()
    pci_resource_len()
    pcim_iomap_table()[i]
    ↓
emu_pcie->region[i]
```

对应代码位置：

- `pcim_enable_device()`：`driver/mt-emu-vgpu.c:240-245`
- `pcim_iomap_regions()`：`driver/mt-emu-vgpu.c:247-252`
- `pci_resource_start()/pci_resource_len()/pcim_iomap_table()`：`driver/mt-emu-vgpu.c:300-308`

最终统一落到：

- `struct emu_region { paddr, vaddr, size }`
- `driver/mt-emu.h:26-30`

也就是说，**CPU 当前能直接访问到的 device 地址窗口，本质上就是 PCI BAR 暴露出来的 MMIO 空间**。

### 20.2 device 侧逻辑地址空间：来自仓库内的固定地址布局约定

除了 BAR 之外，代码里还有大量 `LADDR_*`、`SIZE_*`、`LVADDR_*` 常量。这些地址不是 Linux PCI 层“枚举出来”的，而是驱动和设备固件共同遵守的一套逻辑地址布局。

核心定义在：

- `driver/mt-emu-drv.h:57-103`

典型地址包括：

- `LADDR_APU`
- `LADDR_MTDMA_LL_WR`
- `LADDR_MTDMA_LL_RD`
- `LADDR_VGPU_BASE`
- `LADDR_VGPU(vf)`
- `LADDR_OUTBOUND`
- `LADDR_P2P`

这部分可以理解成：

- BAR 告诉主机“能从哪里碰到设备窗口”
- `LADDR_*` 告诉驱动“设备内部把 DDR / outbound / VF / DMA link-list 逻辑上摆在什么位置”

### 20.3 驱动如何把逻辑地址空间变成可用通道

当前仓库主要靠三类机制把这些逻辑地址接起来。

#### 1) IATU：把 inbound / outbound 地址窗编程进去

IATU 相关 helper 在：

- `driver/mt-emu.h:134-188`

包括：

- `pcie_prog_inbound_atu()`
- `pcie_prog_inbound_atu_vf()`
- `pcie_prog_outbound_atu()`

GPU PF 里 `iatu_init()` 会把：

- PF0 BAR2
- PF1 BAR2
- outbound window
- P2P window
- VF inbound window

这些地址窗配置好：

- `driver/mt-emu-gpu.c:309-339`

#### 2) DMA buffer：为 host/device 共享数据面准备物理区域

`emu_dmabuf_probe()` 负责提供 DMA 可用的共享缓冲区：

- `driver/mt-emu-dmabuf.c:193-255`

它有两种模式：

1. `DMA_RESV_MEM=1`
   - 用固定物理地址
   - `request_mem_region()`
   - `ioremap()`
2. `DMA_RESV_MEM=0`
   - 用 `dma_alloc_coherent()`

这块区域后面会被 DMA 和用户态 `/dev/mt_emu_dmabuf` 共同使用。

#### 3) DMA 通道信息：把 BAR 寄存器窗口和逻辑地址拼成可执行通道

DMA bare 路径里：

- `build_dma_info()`
- `build_dma_info_vf()`

负责构造：

- 通道寄存器地址 `rg_vaddr`
- 链表逻辑地址 `ll_laddr`
- 链表虚拟地址 `ll_vaddr`
- host 侧链表地址 `ll_vaddr_system`

对应代码在：

- `driver/mt-emu-mtdma-bare.c:70-150`

所以从代码视角看，当前 device 的“可用地址空间”并不是单一来源，而是：

- **BAR 资源**：Linux PCI 子系统给出的 host-visible 窗口
- **逻辑地址布局**：驱动和设备约定好的 device-local 地址图
- **IATU/MMU/EATA/DMA**：负责把这两套地址真正接通

### 20.4 用户态是怎么知道这些地址的

当前用户态测试栈并不是直接写死 BAR 值，而是先读驱动暴露的 sysfs 元数据。

驱动侧：

- `show_bar*()` 输出 `paddr:vaddr:size`
- GPU：`driver/mt-emu-gpu.c:61-101`
- vGPU：`driver/mt-emu-vgpu.c:64-90`

用户态侧：

- 读 `/sys/class/misc/<device>/bar*`
- 打开 `/dev/mt_emu_gpu`、`/dev/mt_emu_dmabuf`、`/dev/mt_emu_vgpu<N>`
- 再对对应 fd 做 `mmap`

参考实现：

- `test/lib/mt_pcie_f.c:79-121`
- `test/lib/mt_pcie_f.c:127-205`

因此可以把当前链路压成一句话：

> **内核通过 PCI BAR 拿到 host-visible 地址窗口，通过仓库内固定逻辑地址布局组织 device-local 空间，再经由 sysfs + `/dev` + `mmap / ioctl` 暴露给测试程序。**

---

## 21. 如果要单独构建“地址轮询”机制，建议如何合入

这里的“地址轮询”不要直接实现成“对任意地址做任意条件轮询”的裸能力。更稳的做法是先明确轮询对象，再把它作为一个受控资源服务层接进来。

### 21.1 先把轮询对象分型

建议先明确要做的是哪一类：

1. **BAR 寄存器轮询**
   - 例如某个 doorbell/status 寄存器等待置位
2. **共享内存 / DMA buffer / completion 区轮询**
   - 例如 ring head / tail、completion word、mailbox memory
3. **地址翻译状态轮询**
   - 例如 IATU/MMU/EATA 配置状态、映射生效状态

这三类对象的安全边界、地址校验方式和等待模型都不同，建议不要混成一个“万能轮询 ioctl”。

### 21.2 推荐的合入位置：内核资源服务层新增 poll 子模块

建议不要把轮询逻辑继续叠到 `mt_test_ioctl()` 里，而是新增独立轮询子模块，例如逻辑上归到：

- `mem/`：如果主要轮询共享内存和地址映射状态
- `dma/`：如果主要轮询 DMA completion / descriptor / queue
- `poll/`：如果希望抽成通用地址观察器

它至少要承担下面几件事：

- 管理轮询对象注册和释放
- 校验地址是否属于白名单范围
- 管理比较条件、超时、轮询周期、并发
- 区分 PF / VF 的可见地址域
- 返回命中、超时、取消、非法地址等结果

### 21.3 强烈建议先做“地址域白名单”

当前仓库已经有直接暴露 BAR / CFG 的测试能力，所以如果再做轮询，必须比现有接口更收敛。

建议只允许轮询下面这些受控范围：

1. `emu_pcie->region[]` 里的指定 BAR 白名单区间
   - 例如 doorbell、status、mailbox 寄存器页
2. `emu_dmabuf` 管理的共享缓冲区
   - 例如 completion 区、共享 ring、测试页
3. DMA 通道已知完成区
   - 由 bare DMA / mtdma engine 明确给出
4. 明确标记为 debug-only 的观察区

不建议：

- 允许任意 BAR offset
- 允许任意物理地址
- 允许任意逻辑地址 `LADDR_*`
- 允许用户态自己传入一段未知地址后无限轮询

### 21.4 不建议的合入方式

从当前代码结构看，下面几种方式都不推荐：

1. **继续往 `MT_IOCTL_BAR_RW` 上叠功能**
   - 会把“测试寄存器访问”和“正式轮询 ABI”进一步耦合
2. **直接扩展 `mt_test_ioctl()` 成轮询总入口**
   - 这个文件已经承担太多测试控制路径
3. **让用户态传裸地址 + 条件 + 无限循环**
   - 无法建立稳定边界，也不利于 PF/VF 隔离

### 21.5 推荐的 ABI 设计方向

如果轮询能力准备长期保留，建议单独定义新的 ABI，而不是复用现有 `MT_IOCTL_BAR_RW` / `MT_IOCTL_CFG_RW`。

更推荐的两种模式是：

#### 方案 A：独立 ioctl 家族

例如把轮询相关 ABI 独立成一组“创建 / 等待 / 查询 / 释放”接口：

- 创建轮询对象
- 启动轮询
- 查询状态
- 停止轮询
- 释放对象

这样可以把参数校验、对象生命周期和错误码定义清楚。

#### 方案 B：`poll/eventfd` + 状态页

如果轮询最终是为了“等完成事件”，更推荐：

- 内核内部做地址观察
- 用户态只等待事件
- 共享状态页只承载结果，不直接承载任意地址访问能力

这样更像正式产品驱动的模型。

### 21.6 对这个仓库的具体落地建议

如果就在当前仓库上演进，建议按下面顺序落：

#### 第一步：先补内核内部地址观察器

先只做内核内部服务，不急着对用户态开放所有能力。至少先明确：

- 轮询对象类型
- 地址白名单模型
- 比较条件（等于/不等于/位掩码/边沿）
- 超时模型
- 睡眠轮询还是 busy poll
- PF/VF 是否共用代码路径

#### 第二步：再补正式用户态入口

按目标用途分开：

- **正式长期能力**
  - 单独 ABI
  - 独立对象生命周期
  - 默认只允许白名单地址域
- **bring-up / debug 能力**
  - 放 `debugfs`
  - 或单独 debug char device

#### 第三步：最后再接入现有 DMA / mailbox / queue 路径

真正让轮询机制被业务复用时，再把它接到：

- DMA completion
- mailbox 状态位
- VF 共享状态页
- PF/VF doorbell

这样轮询机制就是“资源服务层能力”，而不是“裸地址调试入口”。

---

## 22. 地址轮询机制的建议模块边界与接口草案

这一节给一个更贴近当前仓库的落地草案，重点是“边界”，不是具体代码写法。

### 22.1 建议新增的内核模块职责

| 建议模块 | 主要职责 | 与现有代码的关系 |
|---|---|---|
| `poll/core` | 轮询对象生命周期、状态机、并发控制 | 新增 |
| `poll/mmio` | BAR 白名单区间校验、MMIO 读值、比较条件 | 依赖 `emu_pcie->region[]` |
| `poll/mem` | 共享内存 / dmabuf / completion 区观察 | 依赖 `emu_dmabuf` |
| `poll/event` | 将轮询命中结果转成等待事件 | 可与现有 completion / waitqueue 协作 |
| `abi/poll_uapi` | 正式轮询请求/结果结构体与 ioctl 编号 | 从现有 `mt-emu-drv.h` 拆分 |
| `debug/poll_debug` | 调试用地址轮询入口 | 仅面向 bring-up |

### 22.2 建议对 PF / VF 分开建白名单

当前仓库里 PF 和 VF 的 BAR 布局、DMA 路径、设备职责并不完全一样，因此建议：

- PF 单独一套白名单表
- VF 单独一套白名单表
- debug-only 区域单独打标

不要只做一张“全设备通用地址表”。

### 22.3 建议的请求语义

建议用户态只描述“要观察什么”，而不是直接描述“内核怎么循环”。

请求语义至少应包含：

- 轮询对象类型
- 目标域 (PF / VF / dmabuf / debug)
- 域内 offset 或命名对象
- 数据宽度 (8 / 16 / 32 / 64)
- 比较条件
- 掩码
- 期望值
- 超时
- 轮询周期

返回语义至少应包含：

- 是否命中
- 最后一次读取值
- 结束原因 (命中 / 超时 / 取消 / 非法地址)

### 22.4 用户态库建议怎么接

当前 `test/lib/mt_pcie_f.c` 可以继续保留为测试原型，但建议不要把正式轮询能力直接塞回这个文件里与 BAR/CFG 裸访问混用。

更稳的做法是：

- 新增独立用户态封装层
- 业务只传“轮询对象 + 条件 + 超时”
- 由用户态库统一组包、等待、错误处理

这样未来即使把底层实现从 busy poll 改成事件驱动，也不会影响业务代码。

### 22.5 一句话合入建议

如果这套地址轮询机制只是为了 bring-up 或临时调试，建议放到 **debug 路径**；

如果它准备长期存在，建议把它放进 **内核资源服务层 + 独立 ABI**，并且从第一版开始就坚持：

- 只允许受控地址域
- PF/VF 分开做白名单
- 不复用 `MT_IOCTL_BAR_RW`
- 不继续扩大 `mt_test_ioctl()` 的职责
