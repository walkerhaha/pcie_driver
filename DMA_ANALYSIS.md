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

## 三、寄存器布局

主要通道寄存器偏移（相对 `rg_vaddr`）：

| 寄存器 | 偏移 | 说明 |
|--------|------|------|
| `REG_DMA_CH_ENABLE` | 0x000 | 通道使能 |
| `REG_DMA_CH_DIRECTION` | 0x004 | 数据/描述符方向控制 |
| `REG_DMA_CH_MMU_ADDR_TYPE` | 0x010 | MMU 地址类型 |
| `REG_DMA_CH_FC` | 0x020 | 流控（flow control） |
| `REG_DMA_CH_INTR_IMSK` | 0x0C4 | 中断屏蔽 |
| `REG_DMA_CH_INTR_RAW` | 0x0C8 | 中断原始状态（读清） |
| `REG_DMA_CH_STATUS` | 0x0D0 | 通道忙状态 |
| `REG_DMA_CH_LBAR_BASIC` | 0x0D4 | 链表基本配置（desc 数量 + chain_en） |
| `REG_DMA_CH_DESC_OPT` | 0x400 | **第一个描述符直接内嵌在寄存器区** |

> 第一个描述符直接写入 `rg_vaddr + 0x400`（即寄存器区内嵌），后续描述符写入链表内存（`ll_vaddr`/`ll_vaddr_system`）。

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
