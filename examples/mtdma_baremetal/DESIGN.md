# 独立裸机 DMA 示例：硬件接口与代码逻辑详解

本目录提供一个**完全独立**的最小化内核模块，演示如何通过裸机模式
（直接操作 MMIO 寄存器，不依赖 Linux DMA engine 框架，不复用原始驱动的任何文件）
驱动 MT EMU PCIe 设备的 MTDMA 控制器完成一次 Host→Device（H2D）和
Device→Host（D2H）数据搬运。

---

## 一、文件说明

| 文件 | 说明 |
|------|------|
| `mtdma_baremetal.h` | 所有硬件寄存器定义、位域宏、描述符结构体、通道状态结构体 |
| `mtdma_baremetal.c` | 独立内核模块：probe/remove、初始化、传输提交、ISR、自测 |
| `Makefile` | 标准内核模块构建文件 |
| `DESIGN.md` | 本文档 |

---

## 二、硬件架构与 PCIe 地址空间

```
主机（x86）                                   MT EMU 设备
 ┌────────────────┐   PCIe 链路    ┌──────────────────────────────────────┐
 │  内核空间       │◄──────────────►│  BAR0  控制寄存器（64MB MMIO）        │
 │  DMA 缓冲区     │                │    0x380000  DMA 公共寄存器区          │
 │  (物理内存)     │                │    0x383000  DMA 通道寄存器区          │
 └────────────────┘                │  BAR2  设备 DDR 访问窗口（MMIO 透传）  │
                                   │    偏移即为设备 DDR 物理地址           │
                                   │  设备 DDR（硬件内存）                  │
                                   │    0x100000  本示例测试数据区          │
                                   │    0x80000000+  DMA 链表区（未使用）   │
                                   └──────────────────────────────────────┘
```

### 2.1 两种通道视角

硬件文档中的通道命名容易混淆：

| 硬件名称 | 数据流向 | PCIe 操作 | 本示例使用 |
|----------|----------|-----------|-----------|
| **RD 通道**（Read Channel） | Host → Device | PCIe MRd（主机发起读请求，设备作为目标） | `rd_ch0`，用于 H2D |
| **WR 通道**（Write Channel） | Device → Host | PCIe MWr（设备发起写请求，主机作为目标） | `wr_ch0`，用于 D2H |

---

## 三、寄存器地址计算

### 3.1 公共寄存器（全局控制）

```
comm_base = BAR0_vaddr + 0x380000
```

| 寄存器 | 偏移（相对 comm_base） | 作用 |
|--------|----------------------|------|
| `REG_COMM_BASIC_PARAM` | `0x000` | RO：硬件版本号 |
| `REG_COMM_CH_NUM` | `0x400` | 配置最大通道数 - 1（必须初始化）|
| `REG_COMM_MST0_BLEN` | `0x408` | AXI Master0（数据路径）突发长度 |
| `REG_COMM_MST1_BLEN` | `0x608` | AXI Master1（描述符 fetch 路径）突发长度 |
| `REG_COMM_ALARM_IMSK` | `0xC00` | 全局告警中断屏蔽，写 0 = 使能 |
| `REG_COMM_RD_IMSK_C32` | `0xC20` | RD 通道 0-31 聚合中断使能（BIT(N)=通道 N）|
| `REG_COMM_WR_IMSK_C32` | `0xC40` | WR 通道 0-31 聚合中断使能 |
| `REG_COMM_RD_STS_C32` | `0xC30` | RD 通道 0-31 完成状态（W1C）|
| `REG_COMM_WR_STS_C32` | `0xC50` | WR 通道 0-31 完成状态（W1C）|
| `REG_COMM_MRG_STS` | `0xC74` | 顶层聚合状态：BIT(0)=RD有完成 BIT(16)=WR有完成 |
| `REG_COMM_WORK_STS` | `0xD00` | 全局忙状态，0 = 全部空闲 |

### 3.2 通道寄存器地址

```
RD 通道 N：rg_base = BAR0_vaddr + 0x383000 + N × 0x1000
WR 通道 N：rg_base = BAR0_vaddr + 0x383000 + N × 0x1000 + 0x800
```

本示例只使用通道 0：

```c
mdev->rd_ch0.rg_base = mdev->bar0 + 0x383000;             // RD ch0
mdev->wr_ch0.rg_base = mdev->bar0 + 0x383000 + 0x800;     // WR ch0
```

### 3.3 通道寄存器布局（相对各通道 rg_base）

```
偏移   寄存器              作用
0x000  REG_CH_ENABLE       通道控制 / 门铃
0x004  REG_CH_DIRECTION    裸机模式方向控制位
0x008  REG_CH_DUMMY_ADDR_L RO：dummy read 目标地址低 32 位
0x00C  REG_CH_DUMMY_ADDR_H RO：dummy read 目标地址高 32 位
0x010  REG_CH_MMU_ADDR_TYPE MMU 地址类型（仅 MTDMA_MMU=1 时有效）
0x020  REG_CH_FC           写通道 FIFO 流控
0x0C4  REG_CH_INTR_IMSK    中断屏蔽，0 = 全部使能
0x0C8  REG_CH_INTR_RAW     中断状态（W1C）
0x0CC  REG_CH_INTR_STATUS  中断屏蔽后状态（只读）
0x0D0  REG_CH_STATUS       通道忙状态 BIT(0)=BUSY
0x0D4  REG_CH_LBAR_BASIC   链表配置 [31:16]=desc_cnt [0]=chain_en

── 第 0 号描述符（内嵌于寄存器区，与 struct mtdma_desc 完全对齐）──
0x400  REG_CH_DESC_OPT     desc_op：BIT(0)=INTR_EN BIT(1)=CHAIN_EN
0x404  REG_CH_ACNT         cnt（传输字节数 - 1）
0x408  REG_CH_SAR_L        源地址低 32 位
0x40C  REG_CH_SAR_H        源地址高 32 位
0x410  REG_CH_DAR_L        目的地址低 32 位
0x414  REG_CH_DAR_H        目的地址高 32 位
0x418  REG_CH_LAR_L        下一描述符物理地址低 32 位
0x41C  REG_CH_LAR_H        下一描述符物理地址高 32 位
```

---

## 四、描述符链机制详解

### 4.1 描述符结构体（`struct mtdma_desc`，32 字节，packed）

```
偏移  字段      说明
 0    desc_op  控制字：BIT(0)=INTR_EN，BIT(1)=CHAIN_EN
 4    cnt      传输字节数 - 1
 8    sar_lo   源地址 [31:0]
12    sar_hi   源地址 [63:32]
16    dar_lo   目的地址 [31:0]
20    dar_hi   目的地址 [63:32]
24    lar_lo   链表下一描述符物理地址 [31:0]
28    lar_hi   链表下一描述符物理地址 [63:32]
```

### 4.2 单描述符模式（本示例使用）

```
第 0 号描述符直接写寄存器区 rg_base + 0x400
desc_op = INTR_EN（BIT(0)=1），无 CHAIN_EN，无 LAR
LBAR_BASIC = 0（chain_en=0，desc_cnt=0）
```

### 4.3 链式描述符模式（扩展参考）

```
第 0 号描述符：写寄存器区 0x400（CHAIN_EN=1，LAR=第1个描述符物理地址）
第 1 号描述符：写链表内存（设备侧 BAR2 MMIO 或主机侧 DMA 内存）
...
第 N 号描述符：CHAIN_EN=0，INTR_EN=1（触发中断）

LBAR_BASIC = (N << 16) | 1
```

---

## 五、传输提交流程（`mtdma_submit_single()`）

```
                        软件操作                      硬件行为
                           │
  1. 写 LBAR_BASIC = 0    ─┤ 配置为单描述符模式
  2. 写 INTR_IMSK = 0     ─┤ 使能所有中断
  3. 写寄存器区 0x400      ─┤ 填入第 0 号描述符
     (DESC_OPT/ACNT/       │   (SAR, DAR, cnt, desc_op=INTR_EN)
      SAR/DAR/LAR)          │
  4. 写 DIRECTION           ─┤ 设置 NOCROSS/DUMMY 等方向控制位
  5. 写 ENABLE=dir_flags   ─┤ 预写（不含 BIT(0)）
  6. 读 ENABLE（read-back）─┤ 强制 PCIe posted write 完成   ← 关键！
  7. 写 ENABLE|BIT(0)      ─┤──────────────────────────────►│ 硬件开始 fetch
                                                             │ 描述符 → 搬运数据
                                                             │ 完成 → 发 MSI 中断
```

**为什么需要 read-back？**
PCIe 写是 posted（单向，不等待确认），主机可能连续写多个寄存器。
如果 ENABLE 写比 DESC_OPT/SAR/DAR 先到达硬件，描述符还没准备好，
硬件就开始执行，会产生 CFG 错误。
读回一次任意寄存器会让主机等待之前所有写完成（读需要等所有写 drain）。

---

## 六、中断处理流程（`mtdma_irq_handler()` → `mtdma_chan_isr()`）

```
MSI 中断触发
     │
     ▼
mtdma_irq_handler()
  ├─ 读 REG_COMM_MRG_STS
  │    BIT(0)=1 → RD 通道有完成
  │    BIT(16)=1 → WR 通道有完成
  │
  ├─ 读 REG_COMM_RD_STS_C32（or WR_STS）
  │    BIT(N)=1 → 通道 N 完成
  │
  └─ 调用 mtdma_chan_isr(ch)
         ├─ val = 读 REG_CH_INTR_RAW      ← 获取原因
         ├─ 解析 val & DONE / ERR_*
         ├─ 写 REG_CH_INTR_RAW = val      ← W1C：清中断
         ├─ 读 REG_CH_INTR_RAW            ← read-back 保证清除完成
         └─ complete(&ch->xfer_done)      ← 唤醒 wait_for_completion_timeout()
```

---

## 七、方向控制位说明

| 传输方向 | SAR 类型 | DAR 类型 | `dir_flags`（写入 REG_CH_DIRECTION / REG_CH_ENABLE）|
|----------|----------|----------|------------------------------------------------------|
| H2D（Host→Device） | 主机物理地址 | 设备 DDR 地址 | `0`（无特殊标志）|
| D2H（Device→Host） | 设备 DDR 地址 | 主机物理地址 | `CH_EN_DUMMY`（BIT(2)，需要 dummy read 保证主机数据可见）|
| H2H（Host→Host）   | 主机物理地址 | 主机物理地址 | `CH_EN_NOCROSS \| CH_EN_DUMMY`（BIT(1)\|BIT(2)）|
| D2D（Device→Device）| 设备地址   | 设备地址   | `CH_EN_NOCROSS`（BIT(1)）|

**Dummy Read 的作用：**
D2H 传输时，DMA 在设备侧完成写入到主机内存后，主机 CPU 可能因为
缓存一致性问题读到旧数据。Dummy read 让 DMA 在完成写入后额外发一次
PCIe 读请求，该读请求完成表示所有写操作已被主机确认，数据可见。

---

## 八、`dma_alloc_coherent` 与 `dma_map_sg` 的选择

本示例使用 `dma_alloc_coherent`（一致性 DMA 内存）：

| 特性 | `dma_alloc_coherent` | `get_user_pages` + `dma_map_sg` |
|------|---------------------|---------------------------------|
| 适用场景 | 内核自己分配 DMA 缓冲区 | 用户态缓冲区（零拷贝）|
| CPU 缓存处理 | 自动保持一致 | 需要 `dma_sync_sg_for_*` |
| 地址连续性 | 物理连续（可直接用物理地址）| 可能不连续（需 scatter-gather）|
| 本示例选用原因 | 简单，无需额外 sync | — |

原始驱动（`emu_dma_rw()`）使用 `get_user_pages_fast` + `dma_map_sg`
实现用户态零拷贝，适合生产环境。本示例优先保持简洁。

---

## 九、完整数据流示意图

```
主机内存（h2d_buf）
 │  物理地址 h2d_bus_addr
 │
 │ ① H2D：RD 通道 0
 │    SAR = h2d_bus_addr（主机物理地址）
 │    DAR = 0x100000（设备 DDR 偏移）
 │    dir_flags = 0
 │    ──────────────────────────────────────────►
 │                                               设备 DDR 0x100000
 │                                               │
 │ ② D2H：WR 通道 0                             │
 │    SAR = 0x100000（设备 DDR 偏移）            │
 │    DAR = d2h_bus_addr（主机物理地址）         │
 │    dir_flags = CH_EN_DUMMY                   │
 │    ◄──────────────────────────────────────────
 │
主机内存（d2h_buf）
 │  物理地址 d2h_bus_addr
 │
 └─ memcmp(h2d_buf, d2h_buf) == 0 → PASS
```

---

## 十、构建与使用

```bash
# 构建
cd examples/mtdma_baremetal
make

# 加载（需要 root 权限，且 MT EMU PCIe 设备已插入）
sudo insmod mtdma_baremetal.ko

# 观察 dmesg 输出
sudo dmesg | grep -E "MTDMA|mtdma"
# 预期输出：
#   MTDMA probe: 1ed5:0200
#   BAR0=... BAR2=...
#   MTDMA hardware version: 0x...
#   MTDMA selftest: H2D  host_bus=0x... → dev=0x100000  size=65536
#   MTDMA: RD channel 0 done
#   MTDMA H2D done OK
#   MTDMA selftest: D2H  dev=0x100000 → host_bus=0x...  size=65536
#   MTDMA: WR channel 0 done
#   MTDMA D2H done OK
#   MTDMA selftest PASSED: H2D/D2H data match

# 卸载
sudo rmmod mtdma_baremetal
```

---

## 十一、与原始驱动的对应关系

| 本示例 | 原始驱动对应位置 |
|--------|-----------------|
| `mtdma_comm_init()` | `driver/mt-emu-mtdma-bare.c: mtdma_comm_init()` |
| `mtdma_chan_init()` | `driver/mt-emu-mtdma-bare.c: mtdma_bare_init()` + `build_dma_info()` |
| `mtdma_submit_single()` | `driver/mt-emu-mtdma-bare.c: dma_bare_xfer()` 单描述符分支 + `driver/mt-emu-mtdma-core.c: mtdma_v0_core_start()` |
| `mtdma_chan_isr()` | `driver/mt-emu-mtdma-bare.c: dma_bare_isr()` |
| `mtdma_irq_handler()` | `driver/mt-emu-ioctl.c: emu_dma_isr()` |
| `mtdma_run_selftest()` | `test/src/mthdma.cc: sanity_dma_bare_single_s` + `dma_bare_move()` |
