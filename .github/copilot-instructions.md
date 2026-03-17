# MT EMU PCIe DMA 驱动 — AI 测试编写技能指南

> 本文件教导 GitHub Copilot 及其他 AI 工具如何为 MT EMU PCIe DMA 内核驱动编写测试用例。
> 内容全部来源于本仓库中**除 `/examples/` 之外**的源文件：`driver/`、`test/`、`scripts/`。

---

## 1. 项目概述

**MT EMU PCIe DMA 驱动**是一套 Linux 内核 + 用户态测试框架，面向基于 Synopsys DesignWare 的 PCIe DMA 控制器（摩尔线程 GPU/APU 设备，厂商 ID `0x1ed5`）。

| 项目 | 值 |
|------|-----|
| 厂商 ID | `0x1ed5` |
| GPU 设备 ID | `0x0200`（QY）、`0x0400`（QY2）、`0x0500`（PH1S）、`0x0680`（HS）、`0x0610`（LS） |
| DMA 写通道数 | 每个 PF 64 条 |
| DMA 读通道数 | 每个 PF 64 条 |
| 最大虚函数数 | 60 |
| 默认 DDR 大小 | 48 GB（可通过 `DDR_SZ_GB` 配置） |

`driver/Makefile` 编译生成的内核模块：

```
mt_emu_gpu.ko   — PF0（GPU）驱动（文件操作、BAR 映射、ioctl、裸机 DMA、中断）
mt_emu_apu.ko   — PF1（APU）驱动
mt_emu_vgpu.ko  — 虚函数驱动（最多 60 个 VF）
mt_emu_mtdma.ko — Linux DMA Engine 封装层
```

---

## 2. 测试框架

所有用户态测试使用 **Catch2**（C++11 单头文件），位于 `test/src/catch2/`。

### 文件布局

```
test/
├── CMakeLists.txt          # 构建配置
├── src/
│   ├── main.cc             # Catch2 入口（全局初始化 pcief_init()）
│   ├── base.cc             # BAR 访问、ROM、SRAM 测试
│   ├── mthdma.cc           # DMA 裸机测试（主测试文件，含 405+ 用例）
│   ├── intr.cc             # 中断 / MSI-X 测试
│   ├── stress.cc           # 压力 / 随机测试
│   ├── test_thr.cc         # 多线程辅助函数实现
│   └── test_thr.h          # 多线程辅助函数声明 + cal_timeout()
└── lib/
    ├── mt_pcie_f.h         # 用户态主 API 头文件（每个测试文件必须包含）
    ├── mt_pcie_f.c         # API 实现
    ├── qy_reg.h            # PCIe 寄存器基地址
    └── simlog.h            # LInfo / LError 日志宏
```

### 每个测试文件必须包含的头文件

```cpp
#include "catch2/catch.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "simlog.h"          // LInfo()、LError()
#include "mt-emu-drv.h"      // ioctl 命令号、枚举、dma_bare_rw 结构体
#include "mt_pcie_f.h"       // 所有用户态 PCIe/DMA API 函数
#include "test_thr.h"        // cal_timeout()、dma_bare_simple_test 辅助函数
```

### 编译与运行

```bash
# 编译驱动模块
cd driver && make

# 编译测试程序
cd test && mkdir -p build && cd build
cmake .. && make

# 按标签运行测试
./test -t "[mtdma0]"      # 冒烟测试（单描述符）
./test -t "[mtdma1]"      # 功能测试（链式、块模式）
./test -t "[mtdma_mmu]"   # MMU+DMA 测试
./test -t "[base]"        # BAR/ROM 测试
./test -t "[intr0]"       # 基础中断测试
./test -h                 # 列出所有标签
```

---

## 3. 设备功能号常量

定义于 `test/lib/mt_pcie_f.h`：

```c
#define F_GPU      0           // PF0 — 主 GPU 功能
#define F_APU      1           // PF1 — APU 功能
#define F_VGUP(i)  (2 + i)    // 第 i 个 VF（i = 0..VF_NUM-1）
#define F_MTDMA    (VF_NUM + 2) // DMA 缓冲区设备
```

---

## 4. 核心 DMA API

### `pcief_dma_bare_xfer` — 执行一次 DMA 传输

```c
int pcief_dma_bare_xfer(
    uint32_t data_direction,   // 数据方向枚举（见第 5 节）
    uint32_t desc_direction,   // 描述符位置：DMA_DESC_IN_DEVICE(0) 或 DMA_DESC_IN_HOST(1)
    uint32_t desc_cnt,         // 描述符总数减一（0 = 单描述符模式）
    uint32_t block_cnt,        // 块数量（0 = 不使用块模式）
    uint32_t ch_num,           // DMA 通道号（0–63）
    uint64_t sar,              // 源地址
    uint64_t dar,              // 目的地址
    uint32_t size,             // 传输字节数
    uint32_t timeout_ms        // 超时毫秒数（推荐用 cal_timeout(size)）
);
// 返回 0 表示成功，非 0 表示失败
```

### `pcief_read` / `pcief_write` — BAR 寄存器访问

```c
int pcief_read (uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data);
int pcief_write(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data);

// 便捷类型化封装（mt_pcie_f.h 中的 inline 函数）：
uint32_t pcief_greg_u32(uint8_t fun, uint8_t bar, uint64_t address);
void     pcief_sreg_u32(uint8_t fun, uint8_t bar, uint64_t address, uint32_t value);
uint64_t pcief_greg_u64(uint8_t fun, uint8_t bar, uint64_t address);
void     pcief_sreg_u64(uint8_t fun, uint8_t bar, uint64_t address, uint64_t value);
```

### DMA 缓冲区分配（主机侧保留内存）

```c
long pcief_dmabuf_malloc(uint64_t len);        // 返回物理地址，失败返回 0
void pcief_dmabuf_free(long addr);
int  pcief_dmabuf_write(uint64_t offset, uint32_t len, void *data);
int  pcief_dmabuf_read (uint64_t offset, uint32_t len, void *data);
```

### 主机/设备信息查询

```c
// 获取 DMA 缓冲区的物理地址、mmap 地址和大小
void pcief_get_mtdma_info(uint64_t *host_paddr, uint64_t *host_maddr, uint64_t *mtdma_size);
```

### 中断辅助函数

```c
int pcief_wait_int(uint8_t fun, int irq, uint32_t *done, uint32_t timeout_ms);
int pcief_trig_int(uint8_t fun, int irq, uint32_t *done);
int pcief_dmaisr_set(uint8_t fun, uint8_t dmabare);
int pcief_irq_init(uint8_t fun, uint8_t type, uint8_t test_mode);
```

---

## 5. 数据方向枚举

```c
// 定义于 driver/mt-emu-drv.h（用户态保护：#ifndef __KERNEL__）
enum dma_transfer_direction {
    DMA_MEM_TO_MEM,   // 0 — 主机到主机（H2H）：SAR 和 DAR 均在主机内存
    DMA_MEM_TO_DEV,   // 1 — 主机到设备（H2D，写）：SAR=主机，DAR=设备 DDR
    DMA_DEV_TO_MEM,   // 2 — 设备到主机（D2H，读）：SAR=设备 DDR，DAR=主机
    DMA_DEV_TO_DEV,   // 3 — 设备到设备（D2D）：SAR 和 DAR 均在设备 DDR
    DMA_TRANS_NONE,
};

// 描述符存放位置
#define DMA_DESC_IN_DEVICE  0   // 描述符存放在设备 DDR（性能最优，推荐）
#define DMA_DESC_IN_HOST    1   // 描述符存放在主机内存（测试主机侧 scatter-gather 时使用）
```

### 如何在单个测试中指定多个方向

使用位掩码：`BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM)`。
`dma_bare_simple_test()` 会遍历所有被置位的方向。

```c
// 同时测试 H2D、D2H、D2D 三个方向
uint32_t dirs = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM) | BIT(DMA_DEV_TO_DEV);
```

---

## 6. 传输模式详解

### 6.1 单描述符传输（最简单）

```
desc_cnt  = 0    （硬件直接从通道寄存器读取 SAR/DAR/ACNT）
block_cnt = 0
```

硬件将 SAR/DAR/ACNT 写入通道寄存器，传输完成后产生一次中断。

### 6.2 链式描述符传输

```
desc_cnt  = N - 1    （N 为描述符总数，例如 32 个描述符则填 31）
block_cnt = 0
```

每个描述符的 `lar` 字段指向设备 DDR 中的下一个描述符。
最后一个描述符的 `lar = 0`，且 `desc_op |= DMA_CH_DESC_BIT_INTR_EN`。
总传输大小 = N × 每段大小 — 将总字节数作为 `size` 传入。

### 6.3 块模式传输

```
desc_cnt  = D - 1    （每块 D 个描述符）
block_cnt = B        （B ≥ 1，块数）
```

将 D 个描述符的链式传输重复 B 次。
总大小 = B × D × 每段大小 — 将总字节数作为 `size` 传入。

---

## 7. 核心辅助函数：`dma_bare_simple_test`

定义于 `test/src/mthdma.cc`，所有 DMA 测试用例均通过此函数运行：

```cpp
static void dma_bare_simple_test(
    uint32_t ch_start_num,        // 起始通道号
    uint32_t ch_cnt,              // 并行通道数
    uint32_t data_direction_bits, // 要测试的 DMA 方向位掩码
    uint32_t desc_direction,      // DMA_DESC_IN_DEVICE 或 DMA_DESC_IN_HOST
    uint32_t desc_cnt,            // N-1 个描述符（0 = 单描述符）
    uint32_t block_cnt,           // 块数量（0 = 不使用块模式）
    uint64_t device_sar,          // 设备侧源地址起始值
    uint64_t device_dar,          // 设备侧目的地址起始值
    uint64_t size,                // 每个通道的总字节数
    int      cnt,                 // 每个线程的重复次数
    int      offset               // 地址偏移
);
```

此函数会为每个通道创建一个独立线程执行传输，并使用 `REQUIRE(*ret == 0)` 断言传输成功。
**多通道测试时，该函数会自动为每个通道递增 `device_sar` 和 `device_dar`（各增加 `size`），无需手动计算地址。**

### 超时计算

```cpp
#define MTDMA_EDK_SPEED  (2*1024*1024)   // 基准速度：2 MB/s

static int cal_timeout(uint64_t size) {
    return ((size * 1000) / MTDMA_EDK_SPEED);
}
// 示例：512KB → 256 ms
```

---

## 8. BAR / 内存访问辅助函数

定义为 `test/lib/mt_pcie_f.h` 中的静态函数，可直接调用无需额外初始化：

```cpp
// 写入后回读比较（验证读写一致性）
static int mem_rw(int fun, int bar, uint64_t offset, uint32_t len);

// 只读
static int mem_read(int fun, int bar, uint64_t offset, uint32_t len);

// 只写
static int mem_write(int fun, int bar, uint64_t offset, uint32_t len);

// 随机地址/长度读写压力测试（8 种对齐 × 4 种位宽）
static int rand_rw(int fun, int bar, uint64_t offset, uint32_t len,
                   uint64_t max_size);
```

---

## 9. 测试标签规范

| 标签 | 适用范围 |
|------|--------|
| `[mtdma0]` | 冒烟测试 — 单描述符、基础方向验证 |
| `[mtdma1]` | 功能测试 — 链式、块模式、各种大小、DDR 地址 |
| `[mtdma_mmu]` | MMU 使能的 DMA 传输 |
| `[base]` | 扩展 ROM 读取验证、BAR 基础读写 |
| `[ph1s_base]` | PH1S BAR4 SRAM 和 BAR2 DDR 读写 |
| `[ph1s_stress]` | PH1S SRAM/DDR 压力测试（随机地址和长度） |
| `[ph_base]` | 通用 BAR0 SRAM 访问 |
| `[ph_stress]` | 随机化多次迭代 SRAM 压力测试 |
| `[intr0]` | 基础中断路由（PF0 软中断触发） |
| `[intr1]` | 高级中断路由（源到目标映射） |
| `[sanity]` | 健全性测试（基础多通道/VF DMA） |
| `[stress]` | 长时间压力测试（含 24 小时） |
| `[perf]` | 性能测试（吞吐量测量） |

---

## 10. 设备 DDR 地址映射

```c
// 来源：driver/mt-emu-drv.h
#define DDR_SZ_GB          48                             // 默认值
#define DDR_SZ             (0x40000000ULL * DDR_SZ_GB)   // 48 GB 总大小
#define DDR_SZ_RESV        0x0040000000ULL               // 1 GB 保留区
#define DDR_SZ_FREE        (DDR_SZ - DDR_SZ_RESV)        // 可用区域顶部

#define LADDR_APU          (DDR_SZ_FREE)                   // APU 起始地址
#define LADDR_MTDMA_LL_WR  (DDR_SZ_FREE + 0x2000000)     // DMA 写通道链表区
#define LADDR_MTDMA_LL_RD  (LADDR_MTDMA_LL_WR + 0x10000) // DMA 读通道链表区
#define LADDR_MTDMA_TEST   0x100000                       // 标准测试区（推荐默认值）

// VF DDR 窗口（最多 60 个 VF，每个 1 GB）
#define SIZE_VGPU_DDR       0x40000000ULL
#define LADDR_VGPU(vf)      (LADDR_VGPU_BASE + SIZE_VGPU_DDR * (vf))
```

---

## 11. DMA 寄存器参考

### 控制器公共寄存器（基地址 `REG_DMA_COMM_BASE = 0x380000`）

| 偏移 | 名称 | 描述 |
|------|------|------|
| 0x000 | BASIC_PARAM | 版本（只读） |
| 0x010 | COMM_ENABLE | 使能 DMA 控制器 |
| 0x400 | CH_NUM | 通道数减一 |
| 0x408 | MST0_BLEN | Master 0 突发长度 |
| 0xC00 | ALARM_IMSK | 告警中断掩码 |
| 0xC04 | ALARM_RAW | 原始告警状态 |
| 0xD00 | WORK_STS | DMA 忙状态 |

### 通道寄存器（基地址 `REG_DMA_CHAN_BASE = 0x383000`，步长 0x1000/通道）

| 偏移 | 名称 | 描述 |
|------|------|------|
| 0x000 | ENABLE | bit0：启动传输 |
| 0x004 | DIRECTION | 地址类型 + 跨设备/本地标志 |
| 0x010 | MMU_ADDR_TYPE | MMU 转换模式 |
| 0x0C4 | INTR_IMSK | 中断掩码 |
| 0x0C8 | INTR_RAW | 原始中断（写 1 清零） |
| 0x0CC | INTR_STATUS | 屏蔽后的中断状态 |
| 0x0D0 | STATUS | bit0：忙 |
| 0x400 | DESC_OPT | bit0：中断使能，bit1：链式使能 |
| 0x404 | ACNT | 字节计数 |
| 0x408 | SAR_L | 源地址 [31:0] |
| 0x40C | SAR_H | 源地址 [63:32] |
| 0x410 | DAR_L | 目的地址 [31:0] |
| 0x414 | DAR_H | 目的地址 [63:32] |
| 0x418 | LAR_L | 链表地址 [31:0] |
| 0x41C | LAR_H | 链表地址 [63:32] |

### 中断位（`REG_DMA_CH_INTR_RAW`）

```c
#define DMA_CH_INTR_BIT_DONE            BIT(0)  // 传输成功完成
#define DMA_CH_INTR_BIT_ERR_DATA        BIT(1)  // 数据错误
#define DMA_CH_INTR_BIT_ERR_DESC_READ   BIT(2)  // 无法读取描述符
#define DMA_CH_INTR_BIT_ERR_CFG         BIT(3)  // 配置错误
#define DMA_CH_INTR_BIT_ERR_DUMMY_READ  BIT(4)  // 地址合法性检查失败
```

---

## 12. `dma_ch_desc` 结构体（28 字节，packed）

```c
// driver/mt-emu-mtdma-bare.h
struct dma_ch_desc {
    uint32_t desc_op;   // BIT(0)=中断使能，BIT(1)=链式使能
    uint32_t cnt;       // 本段字节计数
    union { uint64_t reg; struct { uint32_t lsb, msb; }; } sar;  // 源地址
    union { uint64_t reg; struct { uint32_t lsb, msb; }; } dar;  // 目的地址
    union { uint64_t reg; struct { uint32_t lsb, msb; }; } lar;  // 下一描述符地址
} __packed;
```

---

## 13. ioctl 底层接口（高级用法）

当需要绕过用户态库直接调用 ioctl 时：

```c
// 来源：driver/mt-emu-drv.h
struct dma_bare_rw {
    uint64_t sar;              // 源地址
    uint64_t dar;              // 目的地址
    uint32_t data_direction;   // 数据方向枚举（0-3）
    uint32_t desc_direction;   // 描述符位置（0=设备，1=主机）
    uint32_t desc_cnt;         // 链式描述符数（0=单描述符）
    uint32_t block_cnt;        // 块数量（0=不使用块模式）
    uint32_t size;             // 传输总字节数
    uint32_t ch_num;           // 通道号（0-63）
    uint32_t timeout_ms;       // 超时毫秒数
};

// ioctl 命令号
#define MT_IOCTL_BAR_RW          // BAR 读写
#define MT_IOCTL_MTDMA_BARE_RW   // 裸机 DMA 传输（核心命令）
#define MT_IOCTL_MTDMA_RW        // DMA Engine 传输
#define MT_IOCTL_WAIT_INT        // 等待中断
#define MT_IOCTL_IRQ_INIT        // 中断初始化
```

---

## 14. 测试用例模板

### 模板 A — 单描述符冒烟测试（最简单）

```cpp
TEST_CASE("my_dma_single_h2d", "[mtdma0]") {
    LInfo("TEST_CASE my_dma_single_h2d start\n");

    uint32_t ch_num              = 0;
    uint32_t ch_cnt              = 1;
    uint32_t data_direction_bits = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM);
    uint32_t desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t desc_cnt            = 0;   // 单描述符
    uint32_t block_cnt           = 0;
    uint64_t device_sar          = 0x0;
    uint64_t device_dar          = 0x0;
    uint64_t size                = 512 * 1024;  // 512 KB
    int      cnt                 = 1;

    dma_bare_simple_test(ch_num, ch_cnt, data_direction_bits,
                         desc_direction, desc_cnt, block_cnt,
                         device_sar, device_dar, size, cnt, 0);

    LInfo("TEST_CASE my_dma_single_h2d done\n");
}
```

### 模板 B — 链式描述符功能测试

```cpp
TEST_CASE("my_dma_chain_32desc", "[mtdma1]") {
    LInfo("TEST_CASE my_dma_chain_32desc start\n");

    const uint32_t N_DESC        = 32;
    uint32_t ch_num              = 0;
    uint32_t ch_cnt              = 1;
    uint32_t data_direction_bits = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM)
                                   | BIT(DMA_DEV_TO_DEV);
    uint32_t desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t desc_cnt            = N_DESC - 1;  // 注意：N-1，不是 N
    uint32_t block_cnt           = 0;
    uint64_t device_sar          = 0x0;
    uint64_t device_dar          = 0x0;
    uint64_t size                = N_DESC * 4 * 1024;  // 每段 4 KB，共 128 KB
    int      cnt                 = 1;

    dma_bare_simple_test(ch_num, ch_cnt, data_direction_bits,
                         desc_direction, desc_cnt, block_cnt,
                         device_sar, device_dar, size, cnt, 0);

    LInfo("TEST_CASE my_dma_chain_32desc done\n");
}
```

### 模板 C — 块模式传输测试

```cpp
TEST_CASE("my_dma_block_32x8", "[mtdma1]") {
    LInfo("TEST_CASE my_dma_block_32x8 start\n");

    uint32_t ch_num              = 0;
    uint32_t ch_cnt              = 1;
    uint32_t data_direction_bits = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM)
                                   | BIT(DMA_DEV_TO_DEV);
    uint32_t desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t block_cnt           = 32;   // 32 块
    uint32_t desc_cnt            = 8;    // 每块 8 个描述符（块模式传递实际数量，非 N-1）
    uint64_t device_sar          = 0x0;
    uint64_t device_dar          = 0x0;
    uint64_t size                = block_cnt * 1024; // 每块 1 KB，共 32 KB
    int      cnt                 = 1;

    dma_bare_simple_test(ch_num, ch_cnt, data_direction_bits,
                         desc_direction, desc_cnt, block_cnt,
                         device_sar, device_dar, size, cnt, 0);

    LInfo("TEST_CASE my_dma_block_32x8 done\n");
}
```

### 模板 D — 多通道并行测试

```cpp
TEST_CASE("my_dma_multi_channel", "[mtdma1]") {
    LInfo("TEST_CASE my_dma_multi_channel start\n");

    uint32_t ch_start            = 0;
    uint32_t ch_cnt              = 4;   // 4 个通道同时运行
    uint32_t data_direction_bits = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM);
    uint32_t desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t desc_cnt            = 0;
    uint32_t block_cnt           = 0;
    uint64_t device_sar          = 0x0;
    uint64_t device_dar          = 0x0;
    uint64_t size                = 64 * 1024;  // 每个通道 64 KB
    int      cnt                 = 1;
    // 注意：dma_bare_simple_test 会自动为每个通道递增地址（各增加 size）

    dma_bare_simple_test(ch_start, ch_cnt, data_direction_bits,
                         desc_direction, desc_cnt, block_cnt,
                         device_sar, device_dar, size, cnt, 0);

    LInfo("TEST_CASE my_dma_multi_channel done\n");
}
```

### 模板 E — BAR 寄存器读写测试

```cpp
TEST_CASE("my_bar0_rw", "[base]") {
    LInfo("TEST_CASE my_bar0_rw start\n");

    // 对 BAR0 偏移 0 处的 4 KB 执行写入后回读比较
    REQUIRE(0 == mem_rw(F_GPU, 0, 0, 4096));

    LInfo("TEST_CASE my_bar0_rw done\n");
}
```

### 模板 F — 直接调用 `pcief_dma_bare_xfer`（不使用辅助线程）

```cpp
TEST_CASE("my_single_h2d_direct", "[mtdma0]") {
    LInfo("TEST_CASE my_single_h2d_direct start\n");

    uint64_t dev_addr = LADDR_MTDMA_TEST;
    uint32_t size     = 256 * 1024;   // 256 KB

    int ret = pcief_dma_bare_xfer(
        DMA_MEM_TO_DEV,        // 数据方向：主机→设备
        DMA_DESC_IN_DEVICE,    // 描述符存放位置
        0,                     // desc_cnt（单描述符）
        0,                     // block_cnt（不使用块模式）
        0,                     // 使用通道 0
        0x0,                   // SAR：主机 DMA 缓冲区基地址
        dev_addr,              // DAR：设备 DDR 地址
        size,
        cal_timeout(size)      // 推荐使用此公式计算超时
    );
    REQUIRE(ret == 0);

    LInfo("TEST_CASE my_single_h2d_direct done\n");
}
```

### 模板 G — VF（虚函数）DMA 测试

```cpp
TEST_CASE("my_vf_dma_single", "[mtdma1]") {
    LInfo("TEST_CASE my_vf_dma_single start\n");

    // 使用 VF0 的 DDR 窗口
    uint64_t vf_addr = LADDR_VGPU(0);
    uint32_t size    = 64 * 1024;  // 64 KB

    int ret = pcief_dma_bare_xfer(
        DMA_MEM_TO_DEV,
        DMA_DESC_IN_DEVICE,
        0, 0, 0,
        0x0,
        vf_addr,
        size,
        cal_timeout(size)
    );
    REQUIRE(ret == 0);

    LInfo("TEST_CASE my_vf_dma_single done\n");
}
```

### 模板 H — 软中断触发与等待测试

```cpp
TEST_CASE("my_pf0_soft_intr", "[intr0]") {
    LInfo("TEST_CASE my_pf0_soft_intr start\n");

    uint32_t done = 0;
    uint32_t irq  = QY_INT_SRC_SGI_PF0_TEST;  // 软中断源

    // 触发中断（先清零再置 1）
    pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 0);
    pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 1);

    // 等待中断到达（5 秒超时）
    pcief_wait_int(F_GPU, irq, &done, 5000);
    pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 0);

    REQUIRE(done == 1);
    LInfo("TEST_CASE my_pf0_soft_intr done\n");
}
```

---

## 15. 常用传输大小参考

现有测试覆盖了以下大小；添加新测试时优先选用这些值：

| 大小 | 字节数 |
|------|-------|
| 1 B | 1 |
| 512 B | 512 |
| 4 KB | 4 × 1024 |
| 64 KB | 64 × 1024 |
| 512 KB | 512 × 1024 |
| 16 MB | 16 × 1024 × 1024 |
| 1 GB | 1024 × 1024 × 1024 |

---

## 16. 压力测试 / 随机化模式

```cpp
// 随机描述符数（最多 500 个）
uint32_t desc_cnt = (uint32_t)rand() % 500;

// 随机大小（4 字节对齐）
uint64_t size = ((uint64_t)rand() % (16 * 1024 * 1024)) & ~3ULL;
if (size == 0) size = 4;

// 随机通道号
uint32_t ch = (uint32_t)rand() % PCIE_DMA_CH_NUM;

// 随机块数（0 表示不使用块模式）
uint32_t block_cnt = (uint32_t)rand() % 32;

// 特殊哨兵值：desc_cnt = 65535 时，线程辅助函数使用随机描述符数
// 超时需乘以 100：cal_timeout(size) * 100
```

---

## 17. 编码规范

1. **不要在测试用例内部调用 `pcief_init()`** — 该函数已在 `main.cc` 的全局 Catch2 会话钩子中调用。
2. 使用 `LInfo()` / `LError()`（来自 `simlog.h`）输出日志，不要用裸 `printf`（尽管 printf 也能工作）。
3. 测试用例名称遵循 `<范围>_<功能>_<变体>` 格式（如 `func_dma_bare_chain_ddr_4KB`）。
4. 使用 `REQUIRE(条件)` 做致命断言（失败后立即终止测试）；`CHECK(条件)` 做非致命断言。
5. 优先使用 `dma_bare_simple_test` 而非直接调用 `pcief_dma_bare_xfer` — 前者自动处理线程、超时计算和断言。
6. **始终**将 `cal_timeout(size)` 作为 `timeout_ms` 参数，避免因超时太短导致的虚假失败。
7. `data_direction_bits` 使用 `BIT()` 宏位掩码，可在一个测试中同时验证多个传输方向。
8. **描述符计数始终为 N-1**（N = 描述符总数），这是硬件约定。
9. `block_cnt = 0` 表示**不使用块模式**（单链或单描述符）。
10. 单通道测试使用通道 0；多通道测试使用 `ch_start=0, ch_cnt=N`。
11. 在 `test/src/` 中新建的 `.cc` 文件会被 CMake 自动包含（`file(GLOB TEST_SOURCES src/*.cc)`）。
12. 优先使用 `DMA_DESC_IN_DEVICE`（0）；仅在测试主机侧 scatter-gather 时才使用 `DMA_DESC_IN_HOST`（1）。
13. 块模式超时 = `cal_timeout(size) * block_cnt * 10`。
14. `desc_cnt = 65535` 是特殊哨兵值，告知线程辅助函数使用随机描述符数；此时超时需乘以 100。
15. 多通道测试无需手动计算地址：`dma_bare_simple_test` 会自动为每个通道将 `device_sar` 和 `device_dar` 各递增 `size`。
16. 新建 DMA 测试的默认设备 DDR 起始地址使用 `LADDR_MTDMA_TEST`（`0x100000`）。
17. 所有 BAR 辅助函数（`mem_rw`、`rand_rw` 等）是 `mt_pcie_f.h` 中的 `static` inline 函数，可直接调用。
18. 压力测试中随机化地址和长度时，必须将其限制在 BAR/DDR 窗口范围内，避免越界访问。

---

## 18. 常见问题与边界情况

| 现象 | 可能原因 | 排查方法 |
|------|----------|---------|
| ioctl 返回 -1（ENOENT） | 设备节点不存在 | `ls /dev/mt_emu*` |
| ioctl 返回 -1（EBUSY） | 通道被其他进程占用 | 等待释放或换一个通道 |
| 传输超时 | 中断未到达（MSI 未初始化） | `dmesg \| grep -E "mtdma\|DMA int"` |
| DMA 错误中断 | SAR/DAR 地址越界 | 检查地址是否在有效 DDR 范围内 |
| 数据校验失败 | 描述符链不完整（`desc_cnt` 偏差） | 确认 `desc_cnt = N - 1`，不是 N |
| `desc_cnt` 填错 | 硬件约定 N-1 | 必须减 1，这是最常见的错误 |
| 块模式总大小不对 | 总大小 = 块数 × 每块描述符数 × 每段大小 | 确认三者之积等于传入的 `size` |
| H2H 两端地址相同 | H2H 时 SAR/DAR 均指向主机缓冲区（0x0） | 两段地址可以相同，驱动内部处理 |
| 多通道地址重叠 | 忘记递增地址 | 使用 `dma_bare_simple_test`（自动递增） |

---

## 19. 内核执行路径（用于调试）

```
用户态 pcief_dma_bare_xfer()
  └─ ioctl(fd, MT_IOCTL_MTDMA_BARE_RW, ...)
       └─ mt_test_ioctl()               [driver/mt-emu-ioctl.c]
            └─ dma_bare_xfer()          [driver/mt-emu-mtdma-bare.c]
                 ├─ 写描述符到寄存器/设备内存链表
                 ├─ SET_CH_32(REG_DMA_CH_ENABLE, 1)  ← 启动 DMA
                 └─ wait_for_completion_timeout()     ← 等待完成中断
                      ↑
                 MSI 中断 → dma_bare_isr()
                      └─ complete(&bare_ch->int_done)
```

---

## 20. 脚本工具参考

| 脚本 | 功能 |
|------|------|
| `scripts/install.sh` | 按正确顺序加载所有内核模块 |
| `scripts/uninstall.sh` | 卸载所有内核模块 |
| `scripts/run_test.sh` | 运行指定测试用例 |
| `scripts/check_device.sh` | 检查设备节点是否存在 |

**驱动加载顺序（不可颠倒）：**
```bash
sudo insmod driver/mt_emu_mtdma.ko
sudo insmod driver/mt_emu_gpu.ko
sudo insmod driver/mt_emu_apu.ko
sudo insmod driver/mt_emu_vgpu.ko
```

**创建的设备节点：**
| 设备节点 | 功能 |
|---------|------|
| `/dev/mt_emu_gpu` | PF0 GPU（裸机模式的主要设备） |
| `/dev/mt_emu_vgpu0` ~ `/dev/mt_emu_vgpu59` | VF 虚拟功能 |
| `/dev/mt_emu_dmabuf` | DMA 缓冲区管理 |
