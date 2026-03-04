# 独立裸机 DMA 示例：硬件接口与代码逻辑详解

本目录提供一个**完全独立**的最小化内核模块，演示如何通过裸机模式
（直接操作 MMIO 寄存器，不依赖 Linux DMA engine 框架，不复用原始驱动的任何文件）
驱动 MT EMU PCIe 设备的 MTDMA 控制器，覆盖六种传输模式：

> 命名约定：**"single/multi chain"** 指使用的**通道对数**（1 对 vs 多对并行）；
> **"chain mode"** 指**描述符链式模式**（多个描述符组成链表）。两者含义不同。

1. **single task in single chain** — 单描述符，单通道对（ch0）
2. **chain mode in single chain** — 链式描述符，单通道对（ch0）
3. **chain block mode in single chain** — 多块链式，单通道对（ch0）
4. **single task in multi chain** — 单描述符，多通道对并行（ch0 + ch1）
5. **chain mode in multi chain** — 链式描述符，多通道对并行
6. **chain block mode in multi chain** — 多块链式，多通道对并行

---

## 一、文件说明

| 文件 | 说明 |
|------|------|
| `mtdma_baremetal.h` | 所有硬件寄存器定义、位域宏、描述符结构体、通道状态结构体 |
| `mtdma_baremetal.c` | 独立内核模块：probe/remove、初始化、传输提交、轮询完成检测、自测 |
| `Makefile` | 标准内核模块构建文件 |
| `DESIGN.md` | 本文档 |

---

## 二、硬件架构与 PCIe 地址空间

支持的设备：

| 设备 | Vendor ID | Device ID |
|------|-----------|-----------|
| QY GPU PF（GPU）  | `0x1ed5` | `0x0200` |
| GPU2              | `0x1ed5` | `0x0400` |
| HS                | `0x1ed5` | `0x0680` |

```
主机（x86）                                   MT EMU 设备
 ┌────────────────┐   PCIe 链路    ┌──────────────────────────────────────┐
 │  内核空间       │◄──────────────►│  BAR0  控制寄存器（64MB MMIO）        │
 │  DMA 缓冲区     │                │    0x30000  DMA 公共寄存器区           │
 │  (物理内存)     │                │    0x33000  DMA 通道寄存器区           │
 └────────────────┘                │  BAR2  设备 DDR 访问窗口（MMIO 透传）  │
                                   │    0x010000000000-0x01006fffffff  数据区       │
                                   │    0x010070000000-0x01007fffffff  描述符链表区  │
                                   │  设备 DDR（硬件内存）                  │
                                   │    0x010000100000  本示例测试数据区    │
                                   └──────────────────────────────────────┘
```

### 2.1 两种通道视角

硬件文档中的通道命名容易混淆：

| 硬件名称 | 数据流向 | PCIe 操作 | 本示例使用 |
|----------|----------|-----------|-----------|
| **RD 通道**（Read Channel） | Host → Device | PCIe MRd（主机发起读请求，设备作为目标） | `rd_ch[i]`，用于 H2D |
| **WR 通道**（Write Channel） | Device → Host | PCIe MWr（设备发起写请求，主机作为目标） | `wr_ch[i]`，用于 D2H |

---

## 三、寄存器地址计算

### 3.1 公共寄存器（全局控制）

```
comm_base = BAR0_vaddr + 0x30000
```

| 寄存器 | 偏移（相对 comm_base） | 作用 |
|--------|----------------------|------|
| `REG_COMM_BASIC_PARAM` | `0x000` | RO：硬件版本号 |
| `REG_COMM_CH_NUM` | `0x400` | 配置最大通道数 - 1（必须初始化）|
| `REG_COMM_MST0_BLEN` | `0x408` | AXI Master0（数据路径）突发长度 |
| `REG_COMM_MST1_BLEN` | `0x608` | AXI Master1（描述符 fetch 路径）突发长度 |
| `REG_COMM_ALARM_IMSK` | `0xC00` | 全局告警中断屏蔽，BIT(N)=1 屏蔽对应告警，写 0xFFFFFFFF = 全部屏蔽 |
| `REG_COMM_RD_IMSK_C32` | `0xC20` | RD 通道 0-31 聚合中断使能（BIT(N)=通道 N）|
| `REG_COMM_WR_IMSK_C32` | `0xC40` | WR 通道 0-31 聚合中断使能 |
| `REG_COMM_RD_STS_C32` | `0xC30` | RD 通道 0-31 完成状态（W1C）|
| `REG_COMM_WR_STS_C32` | `0xC50` | WR 通道 0-31 完成状态（W1C）|
| `REG_COMM_MRG_IMSK` | `0xC70` | 顶层聚合中断屏蔽：BIT(0)=RD BIT(16)=WR |
| `REG_COMM_MRG_STS` | `0xC74` | 顶层聚合状态：BIT(0)=RD有完成 BIT(16)=WR有完成 |
| `REG_COMM_WORK_STS` | `0xD00` | 全局忙状态，0 = 全部空闲 |

### 3.2 通道寄存器地址

```
RD 通道 N：rg_base = BAR0_vaddr + 0x33000 + N × 0x1000
WR 通道 N：rg_base = BAR0_vaddr + 0x33000 + N × 0x1000 + 0x800
```

本示例初始化 `MTDMA_NUM_TEST_CH`（= 2）对通道：

```c
// RD 通道 i（Host→Device）
mdev->rd_ch[i].rg_base = mdev->bar0 + 0x33000 + i * 0x1000;

// WR 通道 i（Device→Host）
mdev->wr_ch[i].rg_base = mdev->bar0 + 0x33000 + i * 0x1000 + 0x800;
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

## 五、单描述符传输提交流程（`mtdma_submit_single()`）

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
                                                             │ 完成 → 置位 INTR_RAW
```

**为什么需要 read-back？**
PCIe 写是 posted（单向，不等待确认），主机可能连续写多个寄存器。
如果 ENABLE 写比 DESC_OPT/SAR/DAR 先到达硬件，描述符还没准备好，
硬件就开始执行，会产生 CFG 错误。
读回一次任意寄存器会让主机等待之前所有写完成（读需要等所有写 drain）。

---

## 五b、链式描述符传输提交流程（`mtdma_submit_chain()`）

链式模式将一次传输拆分为 `MTDMA_CHAIN_DESC_NUM`（= 4）个描述符，
每个描述符搬运数据的均等分段。描述符布局如下：

```
寄存器区（BAR0）                     BAR2 描述符链表区（设备 DDR）
 ┌────────────────────────────┐       ┌──────────────────────────────────┐
 │  desc 0  @ rg_base+0x400  │       │  desc 1  @ ll_vaddr+(0)*32       │
 │  desc_op = CHAIN_EN        │       │  desc_op = CHAIN_EN              │
 │  LAR = ll_ddr + 0*32  ────►│──────►│  LAR = ll_ddr + 1*32  ─────────► desc 2 ...
 └────────────────────────────┘       │  desc 2  @ ll_vaddr+(1)*32       │
                                      │  desc_op = CHAIN_EN              │
                                      │  LAR = ll_ddr + 2*32             │
                                      │  ...                             │
                                      │  desc N  @ ll_vaddr+(N-1)*32     │
                                      │  desc_op = INTR_EN               │
                                      │  LAR = 0（链表结束）             │
                                      └──────────────────────────────────┘
```

- `ll_vaddr`：`ch_ll_vaddr(mdev, ch_idx, is_wr)`，BAR2 描述符链表区的内核虚拟地址
- `ll_ddr`：`ch_ll_ddr(ch_idx, is_wr)`，对应的设备 DDR 物理地址（写入 LAR 字段）
- `LBAR_BASIC = (extra_descs << 16) | 1`，其中 `extra_descs = MTDMA_CHAIN_DESC_NUM - 1`

**BAR2 描述符链表区布局（按通道对，stride = `MTDMA_LL_CH_STRIDE` = 64 KB）：**

```
BAR2 offset = MTDMA_DESC_LIST_BASE + (2*ch_idx + is_wr) * MTDMA_LL_CH_STRIDE
```

| 槽 | BAR2 偏移 | 用途 |
|----|----------|------|
| rd_ch[0] | `0x70000000 + 0 * 0x10000` | 通道 0 RD 链表区 |
| wr_ch[0] | `0x70000000 + 1 * 0x10000` | 通道 0 WR 链表区 |
| rd_ch[1] | `0x70000000 + 2 * 0x10000` | 通道 1 RD 链表区 |
| wr_ch[1] | `0x70000000 + 3 * 0x10000` | 通道 1 WR 链表区 |

---

## 六、完成检测方式：寄存器轮询（`mtdma_poll_done()`）

当前版本使用**寄存器轮询**替代 MSI 中断，用于在中断功能不可用时验证
DMA 传输是否正确工作。

```
提交传输（mtdma_submit_single / mtdma_submit_chain）
     │
     ▼
mtdma_poll_done(ch)                         REG_CH_INTR_RAW
  ┌─ 循环轮询，每 500 µs 读一次 ─────────────►│
  │                                           │  0: 传输未完成，继续等待
  │  val = REG_CH_INTR_RAW                   │  BIT(0)=CH_INTR_DONE：传输完成
  │  BIT(0) = CH_INTR_DONE?  ── 是 ──►  W1C 清除，return 0
  │  val & CH_INTR_ERR_MASK? ── 是 ──►  W1C 清除，return -EIO
  │  超时（5 秒）             ── 是 ──►  return -ETIMEDOUT
  └─ 否：usleep_range(500us, 1ms)，继续
```

**为什么仍保留 `DESC_INTR_EN` 和 `REG_CH_INTR_IMSK=0`？**

- `DESC_INTR_EN`（描述符控制字 BIT(0)）：通知硬件在此描述符执行完毕后
  将 `REG_CH_INTR_RAW[0]`（CH_INTR_DONE）**置位**。没有此标志，done 位
  可能不会被置位，轮询将无法检测到完成。
- `REG_CH_INTR_IMSK=0`：不屏蔽通道内部中断状态，确保 INTR_RAW 中的状态
  位可以被软件读取。
- `REG_COMM_RD/WR_IMSK_C32=0`：在聚合层屏蔽通道 0 的完成通知，防止
  发出 MSI（即使 MSI 向量已申请，也不会触发）。

**恢复中断模式的方法**（未来中断修复后）：

1. 将 `REG_COMM_RD/WR_IMSK_C32` 写为 `BIT(0)`（使能通道 0 聚合中断）。
2. 在 `probe` 中调用 `pci_alloc_irq_vectors` + `request_irq` 注册 ISR。
3. 在 ISR 中读 `REG_COMM_MRG_STS` → `RD/WR_STS_C32`，调用
   `complete(&ch->xfer_done)` 唤醒等待任务。
4. 将 `mtdma_run_selftest` 中的 `mtdma_poll_done()` 替换回
   `wait_for_completion_timeout()`。

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

## 九、六种测试用例与数据流

`mtdma_run_selftest()` 依次运行以下 6 个测试，每个测试均以 H2D→D2H 往返
并 `memcmp` 校验作为 PASS/FAIL 判断：

| 测试 | 函数 | 描述符模式 | 并行通道数 | 每块大小 | 块数 |
|------|------|-----------|----------|---------|-----|
| 1 | `test_single_1ch` | 单描述符 | 1（ch0）| 64 KB | 1 |
| 2 | `test_chain_1ch`  | 链式（4 descs）| 1（ch0）| 64 KB | 1 |
| 3 | `test_block_1ch`  | 链式（4 descs）| 1（ch0）| 64 KB/块 | 4 |
| 4 | `test_single_2ch` | 单描述符 | 2（ch0+ch1 并行）| 64 KB | 1 |
| 5 | `test_chain_2ch`  | 链式（4 descs）| 2（ch0+ch1 并行）| 64 KB | 1 |
| 6 | `test_block_2ch`  | 链式（4 descs）| 2（ch0+ch1 并行）| 64 KB/块 | 4 |

**设备 DDR 数据区地址分配（`ch_dev_addr(ch_idx, block_idx)`）：**

```
ch_dev_addr(ch_idx, block_idx) = DEVICE_DATA_BASE
                                 + ch_idx  * (XFER_SIZE * MTDMA_BLOCK_CNT)
                                 + block_idx * XFER_SIZE

DEVICE_DATA_BASE = 0x010000100000
XFER_SIZE        = 64 KB = 0x10000
MTDMA_BLOCK_CNT  = 4
```

**单通道单描述符（Test 1）数据流：**

```
主机内存（h2d_buf）                             设备 DDR
 │  物理地址 h2d_bus_addr                       │ ch_dev_addr(0, 0)
 │                                             │
 │ ① H2D: rd_ch[0], SAR=h2d_bus, DAR=dev      │
 │    dir_flags=0                              │
 │    ──────────────────────────────────────► │
 │                                             │
 │ ② D2H: wr_ch[0], SAR=dev, DAR=d2h_bus      │
 │    dir_flags=CH_EN_DUMMY                    │
 │ ◄────────────────────────────────────────── │
 │
主机内存（d2h_buf）
 └─ memcmp(h2d_buf, d2h_buf) == 0 → Test 1 PASSED
```

**双通道并行（Test 4/5/6）数据流：**

```
  ch0:  h2d_buf[0] ──H2D──► dev(ch=0) ──D2H──► d2h_buf[0]  ──► PASS
  ch1:  h2d_buf[1] ──H2D──► dev(ch=1) ──D2H──► d2h_buf[1]  ──► PASS
         （先同时提交所有通道的 H2D，轮询全部完成后，再同时提交 D2H）
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
sudo dmesg | grep -E "MTDMA|mtdma|Test"
# 预期输出：
#   MTDMA probe: 1ed5:0200
#   BAR0=... BAR2=...
#   MTDMA hardware version: 0x...
#   MTDMA: ch0 rd rg_base=...  wr rg_base=...
#   MTDMA: ch1 rd rg_base=...  wr rg_base=...
#   Test 1: single task in single chain (size=65536)
#   Test 1 PASSED
#   Test 2: chain mode in single chain (4 descs, size=65536)
#   Test 2 PASSED
#   Test 3: chain block mode in single chain (4 blocks x 4 descs, size=65536/block)
#   Test 3 PASSED
#   Test 4: single task in multi chain (2 channels, size=65536)
#   Test 4 PASSED
#   Test 5: chain mode in multi chain (2 channels, 4 descs, size=65536)
#   Test 5 PASSED
#   Test 6: chain block mode in multi chain (2 channels, 4 blocks x 4 descs)
#   Test 6 PASSED

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
| `mtdma_submit_chain()` | `driver/mt-emu-mtdma-bare.c: dma_bare_xfer()` 链式描述符分支 |
| `mtdma_poll_done()` | — （原始驱动使用中断 + `wait_for_completion_timeout()`，本示例用轮询替代）|
| `mtdma_run_selftest()` | `test/src/mthdma.cc: sanity_dma_bare_single_s / sanity_dma_bare_chain_s / sanity_dma_bare_block_s` |
