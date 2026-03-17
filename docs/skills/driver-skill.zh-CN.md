---
title: "Skill：PCIe & DMA 驱动重建"
version: "1.1"
last_modified: "2026-03-17"
dependencies:
  - "environment-skill.zh-CN.md"
target_audience: "AI Agent / Project Replicator"
maintenance_checklist_complete: false
status: "usable"
source_scope: "driver/"
excludes:
  - "examples/"
---

# Skill：PCIe & DMA 驱动重建

## 1. 目标

基于当前仓库 `driver/` 的实现，抽取一套**可迁移的驱动知识契约**，让其它项目在**不继承任何现有源码**的前提下，仍可从零重建：

- PCIe PF/VF 测试驱动
- DMA bare 模式传输层
- DMA engine 封装层
- 用户态 IOCTL 桥接层
- DMA 缓冲区导出层

本 skill **不使用也不引用 `examples/` 目录**。

---

## 2. 驱动总架构

当前驱动可以拆成 5 层：

1. **PCIe 设备前端层**
   - `driver/mt-emu-gpu.c`
   - `driver/mt-emu-apu.c`
   - `driver/mt-emu-vgpu.c`
   - 负责 `probe/remove`、BAR 映射、misc 设备注册、中断初始化。

2. **公共运行时状态层**
   - `driver/mt-emu.h`
   - `driver/mt-emu-drv.h`
   - 定义 `struct emu_pcie`、DMA 地址规划、IOCTL 编号、DMA 参数结构。

3. **用户态接口桥接层**
   - `driver/mt-emu-ioctl.c`
   - `driver/mt-emu-ioctl.h`
   - 负责 `read/write/mmap/ioctl`，把用户态请求分发到 BAR/CFG/中断/DMA。

4. **DMA 执行层**
   - `driver/mt-emu-mtdma-bare.h`
   - `driver/mt-emu-mtdma-bare.c`
   - 负责直接拼装描述符、启动通道、等待中断、判断错误。

5. **DMA 缓冲区与 DMA engine 层**
   - `driver/mt-emu-dmabuf.h`
   - `driver/mt-emu-dmabuf.c`
   - `driver/mt-emu-mtdma-core.h`
   - `driver/mt-emu-mtdma-core.c`
   - 分别负责导出 `/dev/mt_emu_dmabuf` 与 DMA engine 风格调度。

---

## 3. 文件职责清单

| 文件 | 角色 | 维护重点 |
|---|---|---|
| `driver/mt-emu-gpu.c` | PF0/GPU 主入口，整合 DMA、dmabuf、misc、SR-IOV | probe 顺序、资源释放顺序 |
| `driver/mt-emu-apu.c` | PF1/APU 测试入口 | BAR 映射、中断初始化、misc 暴露 |
| `driver/mt-emu-vgpu.c` | VF 入口 | VF 设备名、VF DMA 通道映射、静态 minor |
| `driver/mt-emu-ioctl.c` | 用户态协议入口 | IOCTL 参数校验、copy_to/from_user、中断桥接 |
| `driver/mt-emu-dmabuf.c` | DMA 大缓冲导出 | 保留内存/一致性内存、mmap 边界检查 |
| `driver/mt-emu-mtdma-bare.c` | bare DMA 直控实现 | 描述符布局、通道寄存器、中断完成/错误 |
| `driver/mt-emu-mtdma-core.c` | DMA engine 风格实现 | chunk/burst 组织、virt-dma 挂接 |
| `driver/mt-emu.h` | 驱动共享结构 | `emu_pcie`/`dma_bare`/`mtdma_info` |
| `driver/mt-emu-drv.h` | 常量与协议头 | 设备 ID、DDR 布局、IOCTL、`struct dma_bare_rw` |

---

## 4. 必须保留的核心数据结构

### 4.1 设备与 DMA 总状态

来源：`driver/mt-emu.h`

- `struct emu_pcie`
  - PCI 设备总状态
  - 关键成员：
    - `type`
    - `pcid`
    - `devfn`
    - `miscdev`
    - `emu_mtdma`
    - `dma_bare`
    - `region[7]`
    - `mtdma_comm_vaddr`
    - `irq_type`
    - `irq_test_mode`
    - `int_done[]`
    - `int_mutex[]`
    - `priv_data`

- `struct dma_bare_ch`
  - 一条 bare DMA 通道的运行态
  - 关键成员：
    - `info`
    - `int_done`
    - `int_mutex`
    - `int_error`
    - `chan_id`

- `struct dma_bare`
  - 包含全部 `wr_ch[]`、`rd_ch[]`

### 4.2 用户态协议结构

来源：`driver/mt-emu-drv.h`

- `struct mt_emu_param`
  - 所有 IOCTL 的统一头
- `struct mtdma_rw`
  - DMA engine 风格请求
- `struct dma_bare_rw`
  - bare DMA 请求

### 4.3 描述符结构

来源：`driver/mt-emu-mtdma-bare.h`

- `struct dma_ch_desc`
  - 字段：
    - `desc_op`
    - `cnt`
    - `sar`
    - `dar`
    - `lar`

---

## 5. 必须显式继承的关键常量

来源：`driver/mt-emu-drv.h`、`driver/mt-emu-mtdma-bare.h`

### 5.1 设备类型与设备名

- `MT_GPU_NAME`
- `MT_APU_NAME`
- `MT_VGPU_NAME`
- `MT_MTDMA_NAME`

### 5.2 DDR / 本地地址布局

- `DDR_SZ`
- `DDR_SZ_RESV`
- `DDR_SZ_FREE`
- `LADDR_APU`
- `LADDR_MTDMA_LL_WR`
- `LADDR_MTDMA_LL_RD`
- `LADDR_VGPU(vf)`
- `LADDR_MTDMA_TEST`
- `MTDMA_BUF_SIZE`

### 5.3 IOCTL 编号

- `MT_IOCTL_BAR_RW`
- `MT_IOCTL_CFG_RW`
- `MT_IOCTL_READ_ROM`
- `MT_IOCTL_SUSPEND`
- `MT_IOCTL_RESUME`
- `MT_IOCTL_GET_POWER`
- `MT_IOCTL_WAIT_INT`
- `MT_IOCTL_IPC`
- `MT_IOCTL_IRQ_INIT`
- `MT_IOCTL_MTDMA_BARE_RW`
- `MT_IOCTL_MTDMA_RW`
- `MT_IOCTL_DMAISR_SET`
- `MT_IOCTL_TRIG_INT`

### 5.4 DMA 方向与描述符放置

- `DMA_MEM_TO_MEM`
- `DMA_MEM_TO_DEV`
- `DMA_DEV_TO_MEM`
- `DMA_DEV_TO_DEV`
- `DMA_DESC_IN_DEVICE`
- `DMA_DESC_IN_HOST`

### 5.5 DMA 中断位

- `DMA_CH_INTR_BIT_DONE`
- `DMA_CH_INTR_BIT_ERR_DATA`
- `DMA_CH_INTR_BIT_ERR_DESC_READ`
- `DMA_CH_INTR_BIT_ERR_CFG`
- `DMA_CH_INTR_BIT_ERR_DUMMY_READ`

---

## 6. 关键接口总表

以下接口是后续人工更新 skill 时必须逐项核对的“公开面”。

### 6.1 bare DMA 接口

来源：`driver/mt-emu-mtdma-bare.h`

```c
void mtdma_comm_init(void __iomem *mtdma_comm_vaddr, int vf_num);
void build_dma_info(void *mtdma_vaddr, uint64_t mtdma_paddr,
                    void __iomem *rg_vaddr, void __iomem *ll_vaddr,
                    u8 vf, u8 wr_ch_cnt, u8 rd_ch_cnt,
                    struct mtdma_info *dma_info);
void build_dma_info_vf(void *mtdma_vaddr, uint64_t mtdma_paddr,
                       void __iomem *rg_vaddr, void __iomem *ll_vaddr,
                       struct mtdma_info *dma_info, int devfn);
void mtdma_bare_init(struct dma_bare *dma_bare, struct mtdma_info *info);
void mtdma_bare_init_vf(struct dma_bare *dma_bare, struct mtdma_info *info, int devfn);
int dma_bare_isr(struct dma_bare_ch *bare_ch);
int dma_bare_xfer(struct dma_bare_ch *bare_ch, uint32_t data_direction,
                  uint32_t desc_direction, uint32_t desc_cnt, uint32_t block_cnt,
                  uint64_t sar, uint64_t dar, uint32_t size,
                  uint32_t ch_num, uint32_t timeout_ms);
```

### 6.2 DMA engine 接口

来源：`driver/mt-emu-mtdma-core.h`

```c
int mtdma_probe(struct mtdma_chip *chip);
int mtdma_remove(struct mtdma_chip *chip);
```

### 6.3 dmabuf 接口

来源：`driver/mt-emu-dmabuf.h`

```c
struct emu_dmabuf *emu_dmabuf_probe(struct pci_dev *pcid);
void emu_dmabuf_remove(struct pci_dev *pcid, struct emu_dmabuf *emu_dmabuf);
```

### 6.4 misc / ioctl 接口

来源：`driver/mt-emu-ioctl.h`

```c
int mt_test_open(struct inode *inode, struct file *file);
ssize_t mt_test_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t mt_test_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
int mt_test_release(struct inode *inode, struct file *file);
int mt_test_mmap(struct file *file, struct vm_area_struct *vma);
long mt_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
```

### 6.5 中断初始化接口

来源：`driver/mt-emu.h`

```c
int irq_init(struct emu_pcie *emu_pcie, int type, int test_mode);
void qy_free_irq(struct emu_pcie *emu_pcie);
```

---

## 7. IOCTL 到内部逻辑的映射

| IOCTL | 用户意图 | 内部落点 |
|---|---|---|
| `MT_IOCTL_BAR_RW` | BAR 空间读写 | `mt_test_ioctl()` 中 `readl/writel/readw/writew/readb/writeb` |
| `MT_IOCTL_CFG_RW` | PCIe config 空间读写 | `pci_read_config_*` / `pci_write_config_*` |
| `MT_IOCTL_WAIT_INT` | 等待一个中断源 | `wait_for_completion_timeout(&emu_pcie->int_done[idx])` |
| `MT_IOCTL_TRIG_INT` | 注入软中断 | PF/VF 中断 mux soft 寄存器 |
| `MT_IOCTL_IPC` | 向 DSP/FEC/SMC 发命令并等响应 | mailbox / SGI + completion |
| `MT_IOCTL_IRQ_INIT` | 设置 Legacy/MSI/MSI-X | `irq_init()` |
| `MT_IOCTL_MTDMA_BARE_RW` | 发起 bare DMA | `dma_bare_xfer()` |
| `MT_IOCTL_MTDMA_RW` | 发起 DMA engine 请求 | `emu_dma_rw()` / `pcief_mtdma_engine_start()` 对应路径 |
| `MT_IOCTL_DMAISR_SET` | 切换 DMA ISR 模式 | `emu_pcie->isr_dmabare = emu_param.b0` |

---

## 8. PF0 / PF1 / VF 的职责边界

### 8.1 PF0 / GPU

来源：`driver/mt-emu-gpu.c`

PF0 是整个 DMA 测试体系的主入口，职责最完整：

1. `pcim_enable_device`
2. `pcim_iomap_regions(BAR0|BAR2|BAR4)`
3. `irq_init(..., IRQ_MSI, 0)`
4. `misc_register("/dev/mt_emu_gpu")`
5. `mtdma_comm_init()`
6. `emu_dmabuf_probe()` -> `/dev/mt_emu_dmabuf`
7. `build_dma_info()`
8. `mtdma_bare_init()`
9. `emu_mtdma_init()` / DMA engine 层
10. 可选 `mt_emu_vf_enable()` 打开 VF

### 8.2 PF1 / APU

来源：`driver/mt-emu-apu.c`

APU 只保留测试前端职责：

- BAR0/BAR2 映射
- 中断初始化
- misc 注册
- 不承担 dmabuf 和主 DMA 体系初始化

### 8.3 VF / VGPU

来源：`driver/mt-emu-vgpu.c`

VF 侧与 PF 的差别：

- 每个 VF 暴露独立 misc 设备
- 通过 `build_dma_info_vf()` 建立单 VF 的通道视图
- 通过 `mtdma_bare_init_vf()` 把 VF 的逻辑通道折叠到本地 `wr_ch[0]/rd_ch[0]`

---

## 9. bare DMA 的内在逻辑

来源：`driver/mt-emu-mtdma-bare.c`

### 9.1 通道初始化逻辑

`mtdma_bare_init()` / `mtdma_bare_init_vf()` 的固定动作：

1. 从 `mtdma_info` 拷贝通道寄存器地址与链表地址
2. 初始化 `completion`
3. 初始化 `mutex`
4. 清理残留 `REG_DMA_CH_INTR_RAW`

### 9.2 传输逻辑

`dma_bare_xfer()` 的核心步骤：

1. 根据 `data_direction` 计算
   - `direction`
   - `addr_type`
   - 是否启用 `DMA_CH_EN_BIT_NOCROSS`
   - 是否启用 `DMA_CH_EN_BIT_DUMMY`
2. 根据 `desc_direction` 决定是否打开 `DMA_CH_EN_BIT_DESC_MST1`
3. 重置 `completion`，清空 `int_error`
4. 按三种模式构造描述符
   - 单描述符：`desc_cnt == 0`
   - 链式：`desc_cnt > 0 && block_cnt == 0`
   - 块模式：`block_cnt > 0`
5. 写：
   - `REG_DMA_CH_DIRECTION`
   - `REG_DMA_CH_MMU_ADDR_TYPE`
   - `REG_DMA_CH_LBAR_BASIC`
   - 描述符区与链表区
6. 清中断并使能通道
7. 等待 `wait_for_completion_timeout`
8. 根据 `int_error` 返回结果

### 9.3 中断逻辑

`dma_bare_isr()` 的固定规则：

- `DONE` -> `int_error = 0`
- 任一错误位 -> `int_error = 1`
- 写回 `REG_DMA_CH_INTR_RAW` 清状态
- `complete(&int_done)`

---

## 10. DMA 中断路由逻辑

来源：`driver/mt-emu-ioctl.c`

### 10.1 总入口

```c
irqreturn_t pcie_th(int irq_nr, void *t);
```

按 `emu_pcie->type` 分派到：

- `pcie_apu_th()`
- `pcie_gpu_th()`
- `pcie_vgpu_th()`

### 10.2 DMA 专用分发

```c
void emu_dma_isr(struct emu_pcie *emu_pcie, uint32_t src);
```

PF 逻辑：

- 读取 `REG_DMA_COMM_MRG_PF0_STS`
- 区分 RD merge / WR merge
- 枚举命中的 channel bit
- 调 `dma_bare_isr()`

VF 逻辑：

- 直接查 VF 的 RD/WR `REG_DMA_CH_INTR_STATUS`
- 触发 `rd_ch[0]` / `wr_ch[0]`

---

## 11. DMA engine 层的逻辑骨架

来源：`driver/mt-emu-mtdma-core.c`

如果未来项目需要保留 dmaengine 风格，最小逻辑单元是：

- `mtdma_burst`
- `mtdma_chunk`
- `mtdma_desc`

核心路径：

1. `mtdma_alloc_desc()`
2. `mtdma_alloc_chunk()`
3. `mtdma_alloc_burst()`
4. `mtdma_v0_core_write_chunk()`
5. `mtdma_v0_core_start()`
6. `mtdma_probe()`
7. `mtdma_remove()`

结论：**bare 模式是最短可重建路径，dmaengine 是进阶封装层。**

---

## 12. 重建时必须保持的设备节点与 sysfs 约定

来源：`driver/mt-emu-gpu.c`、`driver/mt-emu-apu.c`、`driver/mt-emu-vgpu.c`、`driver/mt-emu-dmabuf.c`

### 12.1 设备节点

- `/dev/mt_emu_gpu`
- `/dev/mt_emu_apu`
- `/dev/mt_emu_vgpu<N>`
- `/dev/mt_emu_dmabuf`

### 12.2 sysfs 属性

- `/sys/class/misc/mt_emu_gpu/version`
- `/sys/class/misc/mt_emu_gpu/bar0`
- `/sys/class/misc/mt_emu_gpu/bar2`
- `/sys/class/misc/mt_emu_dmabuf/version`
- `/sys/class/misc/mt_emu_dmabuf/bar0`

这些属性被用户态 `pcief_init()` 用来发现 BAR 物理地址、虚拟地址和大小。

---

## 13. 重建顺序模板

未来项目如果只继承本 skill，建议按以下顺序从零重建：

1. 定义 `mt-emu-drv.h` 风格的常量、设备 ID、IOCTL、地址布局
2. 定义 `mt-emu.h` 风格的总状态结构
3. 实现 PF 前端驱动
4. 实现 misc + ioctl
5. 实现 dmabuf 导出
6. 实现 bare DMA 描述符编排与 ISR
7. 最后再补 DMA engine 封装

---

## 14. 人工更新检查表

- [ ] `struct emu_pcie` 成员是否变化
- [ ] `struct dma_bare_rw` 是否变化
- [ ] IOCTL 编号是否变化
- [ ] `dma_bare_xfer()` 参数语义是否变化
- [ ] PF/VF 通道映射是否变化
- [ ] misc 设备名与 sysfs 属性是否变化
- [ ] DMA merge 中断寄存器是否变化
