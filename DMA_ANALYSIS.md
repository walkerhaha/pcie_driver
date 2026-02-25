# DMA 数据搬运机制分析

本文档详细分析 MT EMU PCIe 驱动中 DMA 控制器（MTDMA，基于 Synopsys DesignWare）的调用流程和数据搬运实现原理。

---

## 一、整体架构

驱动实现了两种 DMA 使用模式：

| 模式 | 核心文件 | ioctl 命令 | 特点 |
|------|----------|------------|------|
| **裸机模式（Bare Metal）** | `mt-emu-mtdma-bare.c` | `MT_IOCTL_MTDMA_BARE_RW` | 直接操作 DMA 寄存器，绕过 Linux DMA 框架 |
| **DMA Engine 框架模式** | `mt-emu-mtdma-core.c` + `mt-emu-mtdma-test.c` | `MT_IOCTL_MTDMA_RW` | 遵循 Linux DMA engine 标准 API |

DMA 控制器硬件具有独立的写通道（WR，Device→Host）和读通道（RD，Host→Device），最多各支持 64 条通道（`MTDMA_MAX_WR_CH` / `MTDMA_MAX_RD_CH`）。

---

## 二、关键数据结构

### 2.1 DMA 描述符 `dma_ch_desc`（`mt-emu-mtdma-bare.h`）

```c
struct dma_ch_desc {
    uint32_t desc_op;  /* 控制字：BIT(0)=中断使能，BIT(1)=链式使能 */
    uint32_t cnt;      /* 传输字节数 - 1 */
    union { uint64_t reg; struct { uint32_t lsb; uint32_t msb; }; } sar; /* 源地址 */
    union { uint64_t reg; struct { uint32_t lsb; uint32_t msb; }; } dar; /* 目的地址 */
    union { uint64_t reg; struct { uint32_t lsb; uint32_t msb; }; } lar; /* 链表下一项地址 */
};
```

每个描述符占 32 字节，包含：源地址（SAR）、目的地址（DAR）、下一描述符地址（LAR）、数据量（cnt）和控制位（desc_op）。

### 2.2 通道信息 `mtdma_chan_info`（`mt-emu.h` / `mt-emu-mtdma-core.h`）

```c
struct mtdma_chan_info {
    void __iomem *rg_vaddr;       /* 通道寄存器虚地址（MMIO） */
    void __iomem *ll_vaddr;       /* 描述符链表在设备侧的虚地址 */
    u64           ll_laddr;       /* 描述符链表在设备侧的物理地址 */
    void         *ll_vaddr_system;/* 描述符链表在主机侧的虚地址 */
    u64           ll_laddr_system;/* 描述符链表在主机侧的物理地址 */
    u32           ll_max;         /* 最大描述符数（65536） */
};
```

### 2.3 裸机通道 `dma_bare_ch`（`mt-emu.h`）

```c
struct dma_bare_ch {
    struct mtdma_chan_info info;    /* 通道寄存器和链表地址信息 */
    struct completion     int_done;/* 中断完成量，用于同步等待 */
    struct mutex          int_mutex;
    u8                    int_error;
    u32                   chan_id;
};
```

---

## 三、描述符模式寄存器参考

所有通道寄存器的偏移均相对于该通道的 `rg_vaddr`（MMIO 基地址，由 `build_dma_info()` 计算得到）。

> **设计特点：** 本硬件采用"第一个描述符内嵌于寄存器区"的设计——寄存器区偏移 `0x400` 起的 32 字节就是第 0 号描述符的物理存放位置，而非一个指向外部描述符的地址指针。后续描述符存放在链表内存（设备侧或主机侧），通过每个描述符中的 `LAR` 字段串联。

---

### 3.1 提交入口（Submit Entry / First Descriptor Address）

本硬件没有独立的 "FETCH_ADDR" 寄存器存放外部描述符地址。取而代之的是：**第 0 号描述符直接写入寄存器区偏移 0x400 起的 8 个 32-bit 寄存器**，硬件固定从此处开始 fetch。

| 寄存器宏 | 偏移 | 等效 `dma_ch_desc` 字段 | 说明 |
|----------|------|------------------------|------|
| `REG_DMA_CH_DESC_OPT` | `0x400` | `desc_op` | 描述符控制字（BIT(1)=`CHAIN_EN`：是否继续 fetch 链表；BIT(0)=`INTR_EN`：最后一个描述符置 1 触发中断） |
| `REG_DMA_CH_ACNT` | `0x404` | `cnt` | 本段传输字节数 − 1（即 `size - 1`） |
| `REG_DMA_CH_SAR_L` | `0x408` | `sar.lsb` | 源地址低 32 位 |
| `REG_DMA_CH_SAR_H` | `0x40C` | `sar.msb` | 源地址高 32 位 |
| `REG_DMA_CH_DAR_L` | `0x410` | `dar.lsb` | 目的地址低 32 位 |
| `REG_DMA_CH_DAR_H` | `0x414` | `dar.msb` | 目的地址高 32 位 |
| `REG_DMA_CH_LAR_L` | `0x418` | `lar.lsb` | **第 1 号描述符的物理地址（低 32 位）**；硬件 fetch 完第 0 号描述符后，若 `CHAIN_EN=1`，跳转至此地址继续 fetch |
| `REG_DMA_CH_LAR_H` | `0x41C` | `lar.msb` | 第 1 号描述符的物理地址（高 32 位） |

**链表配置寄存器（必须在写入第 0 号描述符前设置）：**

| 寄存器宏 | 偏移 | 位域 | 说明 |
|----------|------|------|------|
| `REG_DMA_CH_LBAR_BASIC` | `0x0D4` | `[31:16]` = 链表中后续描述符总数（即 `desc_cnt`，0 表示单描述符模式） | 告知硬件链表长度，用于预分配 fetch 资源 |
| | | `[0]` = `chain_en`（0 = 单描述符，1 = 链式模式） | 0 时硬件只执行第 0 号描述符并停止 |

驱动中的写入顺序（`dma_bare_xfer()`）：
```c
// 1. 配置链表参数
SET_CH_32(bare_ch, REG_DMA_CH_LBAR_BASIC, (desc_cnt << 16) | chain_en);

// 2. 写第 0 号描述符（直接写寄存器区 0x400）
lli = bare_ch->info.rg_vaddr + REG_DMA_CH_DESC_OPT;  // = rg_vaddr + 0x400
SET_LL_32(lli, cnt,     size - 1);
SET_LL_32(lli, sar.lsb, lower_32_bits(sar));
SET_LL_32(lli, sar.msb, upper_32_bits(sar));
SET_LL_32(lli, dar.lsb, lower_32_bits(dar));
SET_LL_32(lli, dar.msb, upper_32_bits(dar));
// 链式时还需写 lar.lsb / lar.msb → 第 1 号描述符的物理地址

// 3. 后续描述符（i >= 1）写入链表内存（设备侧或主机侧），不再走寄存器
lli = &(((struct dma_ch_desc *)bare_ch->info.ll_vaddr)[i - 1]);
// lar 字段 = ll_laddr + i * 32  （指向下一个描述符的本地物理地址）
```

---

### 3.2 启动 / 门铃（Start / Doorbell）

| 寄存器宏 | 偏移 | 位域 | 说明 |
|----------|------|------|------|
| `REG_DMA_CH_ENABLE` | `0x000` | `[0]` = `ENABLE`（写 1 启动，写 0 预清） | **门铃寄存器**：写 1 后 DMA 引擎立即开始 fetch 第 0 号描述符并搬运数据 |
| | | `[1]` = `NOCROSS`（仅 H2H/D2D 时置 1，禁止跨 PCIe 边界） | 方向控制辅助位 |
| | | `[2]` = `DUMMY`（D2H/H2H 时置 1，触发 dummy read 保证数据可见性） | |
| `REG_DMA_CH_DIRECTION` | `0x004` | `[1]` = `NOCROSS`；`[2]` = `DUMMY`；`[0]` = `DESC_MST1`（描述符走 PCIe master 1） | 链式模式下的方向控制（裸机模式写此寄存器；DMA Engine 模式通过 `ch_en` 隐含） |

**标准启动序列（`mtdma_v0_core_start()` / `dma_bare_xfer()`）：**
```c
// Step 1: 先写 0 清除旧使能状态（可选，裸机模式在最后统一写）
SET_CH_32(chan, REG_DMA_CH_ENABLE, ch_en_flags);   // 设置方向标志但不置 ENABLE

// Step 2: 确保前序 PCIe 写已到达硬件（读回确认）
GET_CH_32(chan, REG_DMA_CH_ENABLE);

// Step 3: 置 ENABLE 位，DMA 引擎开始 fetch 第 0 号描述符
SET_CH_32(chan, REG_DMA_CH_ENABLE, ch_en_flags | DMA_CH_EN_BIT_ENABLE);  // BIT(0) = 1
```

> 注意：`REG_DMA_CH_FC`（`0x020`）也需在启动前配置（DMA Engine 模式中：`BIT(0) | (WCH_FC_THLD<<1)`），用于控制写通道 FIFO 水位，防止 PCIe 背压导致 hang。

---

### 3.3 完成判断（Completion Status）

#### 3.3.1 轮询方式（Polling）

| 寄存器宏 | 偏移 | 位域 | 说明 |
|----------|------|------|------|
| `REG_DMA_CH_STATUS` | `0x0D0` | `[0]` = `BUSY`（1=传输中，0=空闲/完成） | **忙/空闲状态位**；`dma_v0_core_ch_status()` 读此寄存器判断通道是否完成 |

```c
// 驱动中的轮询判断（mtdma_v0_core_ch_status）
tmp = GET_CH_32(chan, REG_DMA_CH_STATUS);
if (tmp & DMA_CH_STATUS_BUSY)   // BIT(0)
    return DMA_IN_PROGRESS;     // 仍在传输
else
    return DMA_COMPLETE;        // 已完成
```

#### 3.3.2 中断方式（Interrupt，推荐）

| 寄存器宏 | 偏移 | 位域 | 说明 |
|----------|------|------|------|
| `REG_DMA_CH_INTR_IMSK` | `0x0C4` | 写 `0x0` = 使能所有中断；写 `0x1F` = 屏蔽所有 | **中断屏蔽**：启动前写 0 开中断，清零 ISR 返回后可再次屏蔽 |
| `REG_DMA_CH_INTR_RAW` | `0x0C8` | `[0]` = `DONE`（写 1 清除） | **传输完成中断**：最后一个描述符（`INTR_EN=1`）执行完后置 1，触发 MSI/MSI-X |
| `REG_DMA_CH_INTR_STATUS` | `0x0CC` | 同 `INTR_RAW` 但经过 mask 过滤 | 可选：读 INTR_STATUS 代替 INTR_RAW（已屏蔽的中断不显示） |

**ISR 中的完成判断（`dma_bare_isr()`）：**
```c
val = GET_CH_32(bare_ch, REG_DMA_CH_INTR_RAW);

if (val & DMA_CH_INTR_BIT_DONE)          // BIT(0)：全部描述符执行完毕
    bare_ch->int_error = 0;              // 正常完成

SET_CH_32(bare_ch, REG_DMA_CH_INTR_RAW, val);  // 写 1 清中断（W1C）
GET_CH_32(bare_ch, REG_DMA_CH_INTR_RAW);       // 读回，确保 PCIe write 已达硬件

complete(&bare_ch->int_done);                  // 唤醒等待者
```

---

### 3.4 错误寄存器（Error Register）

#### 通道级错误：`REG_DMA_CH_INTR_RAW`（`0x0C8`）

错误位与完成位共用同一寄存器（W1C，写 1 清除）：

| 位 | 宏定义 | 错误类型 | 典型原因 |
|----|--------|----------|----------|
| `[0]` | `DMA_CH_INTR_BIT_DONE` | 完成（非错误） | 正常完成 |
| `[1]` | `DMA_CH_INTR_BIT_ERR_DATA` | **数据错误** | PCIe completion abort / AXI slave error（数据传输阶段） |
| `[2]` | `DMA_CH_INTR_BIT_ERR_DESC_READ` | **描述符 fetch 错误** | LAR 指向的地址不可达、PCIe 链路异常、权限错误（描述符读取阶段）|
| `[3]` | `DMA_CH_INTR_BIT_ERR_CFG` | **配置错误** | `LBAR_BASIC`、地址对齐、描述符字段非法等配置不合法 |
| `[4]` | `DMA_CH_INTR_BIT_ERR_DUMMY_READ` | **Dummy read 错误** | D2H/H2H 模式下用于数据可见性的虚读操作失败 |

**ISR 中的错误解析：**
```c
val = GET_CH_32(bare_ch, REG_DMA_CH_INTR_RAW);

if (val & DMA_CH_INTR_BIT_ERR_DATA)
    printk("DMA DATA error: PCIe/AXI error during data transfer (val=0x%x)\n", val);
if (val & DMA_CH_INTR_BIT_ERR_DESC_READ)
    printk("DMA DESC_READ error: failed to fetch descriptor via LAR (val=0x%x)\n", val);
if (val & DMA_CH_INTR_BIT_ERR_CFG)
    printk("DMA CFG error: illegal descriptor config or alignment (val=0x%x)\n", val);
if (val & DMA_CH_INTR_BIT_ERR_DUMMY_READ)
    printk("DMA DUMMY_READ error: dummy read failed (val=0x%x)\n", val);
```

#### 全局/公共错误：`REG_DMA_COMM_COMM_ALARM_RAW`（公共基址 `+0xC04`）

| 寄存器宏 | 偏移（相对公共基址）| 说明 |
|----------|-------------------|------|
| `REG_DMA_COMM_COMM_ALARM_IMSK` | `0xC00` | 全局告警中断屏蔽（写 0 使能） |
| `REG_DMA_COMM_COMM_ALARM_RAW` | `0xC04` | 全局告警原始状态（跨通道 AXI/PCIe 级错误） |
| `REG_DMA_COMM_COMM_ALARM_STATUS` | `0xC08` | 全局告警屏蔽后状态 |
| `REG_DMA_COMM_WORK_STS` | `0xD00` | 全局忙状态（初始化时检测，非零表示 DMA 未完全空闲） |

---

### 3.5 辅助寄存器（Auxiliary）

| 寄存器宏 | 偏移 | 说明 |
|----------|------|------|
| `REG_DMA_CH_DIRECTION` | `0x004` | 裸机模式下的方向寄存器（`NOCROSS`/`DUMMY`/`DESC_MST1`），DMA Engine 模式合并进 `REG_DMA_CH_ENABLE` |
| `REG_DMA_CH_MMU_ADDR_TYPE` | `0x010` | 仅 `MTDMA_MMU=1` 时有效：地址类型（H2D=`0x100`，D2H=`0x1`，D2D=`0x101`，H2H=`0x0`） |
| `REG_DMA_CH_FC` | `0x020` | 写通道 FIFO 流控：`BIT(0)=FC_EN`，`[x:1]=THLD`（阈值，默认 `0x10` = 16K） |
| `REG_DUMMY_CH_ADDR_L/H` | `0x008/0x00C` | **只读**；D2H/H2H 模式下硬件自动填充的 dummy read 目标地址（调试用） |

---

## 四、初始化流程

```
PCIe probe
  └─ mtdma_comm_init()         // 初始化公共寄存器：通道数、突发长度、中断屏蔽
  └─ build_dma_info()          // 计算每条通道的寄存器地址和链表缓冲区地址
  └─ mtdma_bare_init()         // 初始化裸机通道：completion / mutex / 清中断
  └─ emu_mtdma_init()          // 初始化 DMA Engine 通道结构
       └─ mtdma_probe()        // 向 Linux DMA engine 框架注册设备
            └─ mtdma_channel_setup()  // 为每条通道注册回调（alloc/free/config/prep/issue）
```

### 4.1 公共寄存器初始化 `mtdma_comm_init()`

```c
SET_COMM_32(base, REG_DMA_COMM_CH_NUM,      PCIE_DMA_CH_NUM - 1);   // 通道总数
SET_COMM_32(base, REG_DMA_COMM_MST0_BLEN,   (MST0_ARLEN<<4)|MST0_AWLEN); // AXI 突发长度
SET_COMM_32(base, REG_DMA_COMM_MST1_BLEN,   (MST1_ARLEN<<4)|MST1_AWLEN);
// 使能各 VF 的中断屏蔽位...
```

### 4.2 通道地址映射 `build_dma_info()`

```
写通道（WR，Device→Host）：
  rg_vaddr  = BAR 寄存器区 + 0x383000 + ch*0x1000 + 0x800
  ll_laddr  = 0x80000000 + ch * 65536 * 32  （设备侧链表起始物理地址）
  ll_vaddr  = BAR MMIO 映射 + ll_laddr

读通道（RD，Host→Device）：
  rg_vaddr  = BAR 寄存器区 + 0x383000 + ch*0x1000
  ll_laddr  = 0x80000000 + wr_ch_cnt*65536*32 + ch*65536*32
```

---

## 五、数据搬运流程

### 5.1 裸机模式（`dma_bare_xfer()`）

用户态通过 `MT_IOCTL_MTDMA_BARE_RW` 传入 `dma_bare_rw` 参数，内核调用 `dma_bare_xfer()`。

#### 5.1.1 单描述符模式（`desc_cnt == 0`）

```
dma_bare_xfer()
  1. 加锁 mutex，reinit_completion(&int_done)
  2. lli = rg_vaddr + REG_DMA_CH_DESC_OPT  // 第一个（唯一）描述符写寄存器
  3. SET_CH_32(REG_DMA_CH_LBAR_BASIC, 0|chain_en=0)
  4. 写描述符字段：cnt, sar.lsb/msb, dar.lsb/msb
  5. SET_CH_32(REG_DMA_CH_INTR_IMSK, 0)    // 开中断
  6. SET_CH_32(REG_DMA_CH_DIRECTION, direction)
  7. SET_CH_32(REG_DMA_CH_ENABLE, DMA_CH_EN_BIT_ENABLE)  // 启动
  8. wait_for_completion_timeout(&int_done, timeout_ms)   // 等中断
  9. 解锁 mutex
```

#### 5.1.2 链式描述符模式（`desc_cnt > 0`）

```
dma_bare_xfer()
  1. 加锁 mutex，reinit_completion(&int_done)
  2. 第 0 个描述符：lli = rg_vaddr + REG_DMA_CH_DESC_OPT（寄存器内嵌）
     SET_CH_32(REG_DMA_CH_LBAR_BASIC, (desc_cnt<<16)|1)  // 链式使能
  3. 第 i>0 个描述符：
     - 若描述符在设备侧：lli = ll_vaddr[i-1]，lar = ll_laddr + i*32
     - 若描述符在主机侧：lli = ll_vaddr_system[i-1]，lar = ll_laddr_system + i*32
     每个描述符的 lar 字段指向下一个描述符的物理地址（形成链表）
     最后一个描述符的 lar 无效（由 desc_op 的 CHAIN_EN=0 终止）
  4. 写方向寄存器、使能寄存器，启动 DMA
  5. wait_for_completion_timeout() 等待中断
```

描述符链结构示意：

```
┌──────────────────────────────┐
│ 寄存器区 REG_DMA_CH_DESC_OPT │  ← 第 0 个描述符
│  desc_op | cnt | SAR | DAR   │
│  LAR → ll_laddr + 32         │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│     ll_vaddr[0] (设备内存)    │  ← 第 1 个描述符
│  desc_op | cnt | SAR | DAR   │
│  LAR → ll_laddr + 64         │
└──────────────┬───────────────┘
               ▼
             ......
               ▼
┌──────────────────────────────┐
│    ll_vaddr[N-1]             │  ← 最后一个描述符
│  desc_op=INTR_EN | cnt|SAR|DAR│  CHAIN_EN=0，触发中断
│  LAR（无效）                  │
└──────────────────────────────┘
```

#### 5.1.3 中断处理 `dma_bare_isr()`

```c
val = GET_CH_32(bare_ch, REG_DMA_CH_INTR_RAW);
if (val & DMA_CH_INTR_BIT_DONE)          // 传输完成
    bare_ch->int_error = 0;
else if (val & DMA_CH_INTR_BIT_ERR_*)    // 出错
    bare_ch->int_error = 1;
SET_CH_32(bare_ch, REG_DMA_CH_INTR_RAW, val);  // 写 1 清中断
GET_CH_32(bare_ch, REG_DMA_CH_INTR_RAW);       // 读回确保 PCIe 写完成
complete(&bare_ch->int_done);                  // 唤醒等待者
```

---

### 5.2 DMA Engine 框架模式（`emu_dma_rw()` + `mtdma_xfer()`）

用户态通过 `MT_IOCTL_MTDMA_RW` 触发，流程如下：

```
emu_dma_rw()
  1. dma_request_chan(dev, "tx"/"rx")        // 从 DMA engine 框架申请通道
  2. get_user_pages_fast(userbuf, ...)       // pin 住用户态缓冲区的物理页
  3. sg_alloc_table() + sg_set_page()       // 构建 scatter-gather 表
  4. dma_map_sg(dev, sgt.sgl, ...)          // IOMMU 映射，获取 DMA 总线地址
  5. down(sem)                              // 通道互斥信号量
  6. for test_cnt 次:
       mtdma_xfer(done, chan, dir, laddr, &sgt)
         ├─ dmaengine_slave_config(chan, &sconf)          // 配置 src/dst 地址
         ├─ dmaengine_prep_slave_sg(chan, sgl, nents, ...) // 准备 SG 描述符
         │    └─ mtdma_device_prep_slave_sg()
         │         └─ mtdma_device_transfer()             // 分配 desc/chunk/burst
         │              └─ vchan_tx_prep()                // 挂入虚通道队列
         ├─ txdesc->callback = mtdma_test_callback        // 注册完成回调
         ├─ dmaengine_submit(txdesc)                      // 提交描述符
         └─ dma_async_issue_pending(chan)                 // 触发传输
              └─ mtdma_device_issue_pending()
                   └─ mtdma_start_transfer()
                        └─ mtdma_v0_core_start()          // 写寄存器，启动硬件
       wait_for_completion_timeout(done, ...)
       dmaengine_terminate_all(chan)
  7. up(sem)
  8. dma_unmap_sg() + sg_free_table() + kfree(dma_pages) + dma_release_channel()
```

#### 5.2.1 DMA Engine 回调链

| 回调 | 实现函数 | 说明 |
|------|----------|------|
| `device_alloc_chan_resources` | `mtdma_alloc_chan_resources()` | 申请通道资源 |
| `device_config` | `mtdma_device_config()` | 保存 slave config（src/dst addr） |
| `device_prep_slave_sg` | `mtdma_device_prep_slave_sg()` | 构建 burst/chunk/desc 链 |
| `device_issue_pending` | `mtdma_device_issue_pending()` | 从虚通道队列取出并启动 |
| `device_tx_status` | `mtdma_device_tx_status()` | 查询传输状态/残余量 |
| `device_terminate_all` | `mtdma_device_terminate_all()` | 中止传输 |

#### 5.2.2 硬件启动 `mtdma_v0_core_start()`

```c
mtdma_v0_core_write_chunk(chunk);              // 把 burst 链写入描述符寄存器/链表
if (dir == DMA_MEM_TO_DEV) ch_en = DMA_CH_EN_BIT_DESC_MST1;
SET_CH_32(chan, REG_DMA_CH_ENABLE, ch_en);
SET_CH_32(chan, REG_DMA_CH_FC, BIT(0)|(WCH_FC_THLD<<1));  // 流控
SET_CH_32(chan, REG_DMA_CH_INTR_IMSK, 0);       // 开中断
SET_CH_32(chan, REG_DMA_CH_INTR_RAW, ...);      // 清旧中断
GET_CH_32(chan, REG_DMA_CH_ENABLE);             // 读回，确保 PCIe 写完成
SET_CH_32(chan, REG_DMA_CH_ENABLE, ch_en | DMA_CH_EN_BIT_ENABLE);  // 启动
```

#### 5.2.3 中断处理 `mtdma_isr()` → `mtdma_done_interrupt()`

```c
// 读中断状态，清中断
status = GET_CH_32(chan, REG_DMA_CH_INTR_RAW);
SET_CH_32(chan, REG_DMA_CH_INTR_RAW, status);

if (status & DMA_CH_INTR_BIT_DONE) {
    // chunks_alloc > 0：还有分片，继续启动下一 chunk
    // chunks_alloc == 0：全部完成，调用 vchan_cookie_complete() 触发回调
}
if (status & DMA_CH_INTR_BIT_ERR_*) {
    mtdma_abort_interrupt(chan);  // 错误处理
}
```

---

## 六、DMA 传输方向

| `data_direction` 值 | 含义 | `ch_en` 控制位 |
|---------------------|------|---------------|
| `DMA_MEM_TO_MEM` | Host→Host（内存到内存） | NOCROSS + DUMMY |
| `DMA_MEM_TO_DEV` | Host→Device（H2D，写设备） | addr_type=0x100 |
| `DMA_DEV_TO_MEM` | Device→Host（D2H，读设备） | DUMMY，addr_type=0x1 |
| `DMA_DEV_TO_DEV` | Device→Device | NOCROSS，addr_type=0x101 |

描述符位置（`desc_direction`）：
- `DMA_DESC_IN_DEVICE`：描述符链表存放在设备内存（通过 MMIO 访问）
- `DMA_DESC_IN_HOST`：描述符链表存放在主机内存（系统内存）

---

## 七、内存布局

```
设备 DDR 地址空间：
  0x0000_0000_0000 ~ DDR_SZ_FREE        GPU/测试数据区
  DDR_SZ_FREE                           APU 区（32MB）
  DDR_SZ_FREE + SIZE_APU_DDR            MTDMA 写通道链表区（LL_WR，64KB×32=2MB/通道）
  LADDR_MTDMA_LL_WR + SIZE_MTDMA_LL_RW  MTDMA 读通道链表区（LL_RD）

主机保留内存（grub.cfg: memmap=32G$32G）：
  32GB ~ 64GB                           DMA 大缓冲区（reserved memory）
```

---

## 八、流程总结图

```
用户态
  │  ioctl(MT_IOCTL_MTDMA_BARE_RW)      ioctl(MT_IOCTL_MTDMA_RW)
  ▼                                      ▼
内核态
  dma_bare_xfer()                       emu_dma_rw()
  │ 构建描述符链（MMIO 写）               │ pin 用户页，建 SG 表，dma_map_sg()
  │ 配置方向/使能寄存器                   │ dmaengine_slave_config/prep/submit
  │ 启动硬件 DMA                         │ dma_async_issue_pending()
  │                                      │   mtdma_v0_core_start()（写寄存器启动）
  ▼                                      ▼
硬件 MTDMA 控制器（Synopsys DW）
  │ 按描述符链搬运数据（PCIe TLP）
  │ 完成后触发 MSI/MSI-X 中断
  ▼
中断处理
  dma_bare_isr()                        mtdma_isr()
  │ 读/清中断                            │ 读/清中断
  │ complete(&int_done)                  │ vchan_cookie_complete() → 触发回调
  ▼                                      ▼
  wait_for_completion_timeout() 返回     mtdma_test_callback() → complete(done)
  返回用户态                             wait_for_completion_timeout() 返回
```
