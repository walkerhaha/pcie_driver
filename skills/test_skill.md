# MT EMU PCIe DMA 驱动 — 测试集构建技能指南

> **用途**：本文件供 AI 工具（GitHub Copilot、Cursor 等）读取，
> 基于此可生成、扩展或修改 `test/` 目录下的用户态测试用例。
> 测试集采用 **Catch2**（C++11 单头文件）框架。
>
> ⚠️  本文件仅覆盖测试集生成，驱动文件与脚本生成请参阅 `skills/driver_skill.md`。
> ⚠️  忽略 `/examples/` 和 `/.github/` 目录下的所有内容。

---

## 1. 测试配置项（切换项目 / 设备时在此处修改）

以下参数在切换项目或测试目标时需要人为更新，AI 工具生成测试代码时应优先引用这里的值。

```yaml
# ────────────────────────────────────────────────────────
#  【必须】默认 DMA 起始通道号与通道数
# ────────────────────────────────────────────────────────
DEFAULT_CH_START: 0          # 单通道测试时使用通道 0
DEFAULT_CH_CNT:   1          # 默认并行通道数

# ────────────────────────────────────────────────────────
#  【必须】DMA 测试默认大小（字节）
#  常用值：512, 4096, 65536, 524288(512KB), 16777216(16MB)
# ────────────────────────────────────────────────────────
DEFAULT_TEST_SIZE: 524288    # 512 KB

# ────────────────────────────────────────────────────────
#  【必须】设备 DDR 测试基地址
#  通常使用 LADDR_MTDMA_TEST = 0x100000
# ────────────────────────────────────────────────────────
DEFAULT_DDR_SAR: "0x0"       # 设备侧源地址（0 = LADDR_MTDMA_TEST 起点）
DEFAULT_DDR_DAR: "0x0"       # 设备侧目的地址（0 = 与 SAR 相同起点）

# ────────────────────────────────────────────────────────
#  【必须】默认测试方向（位掩码，可叠加）
#  DMA_MEM_TO_DEV=1, DMA_DEV_TO_MEM=2, DMA_DEV_TO_DEV=4, DMA_MEM_TO_MEM=1（枚举值）
#  BIT(1)|BIT(2) = H2D + D2H
# ────────────────────────────────────────────────────────
DEFAULT_DIRECTION_BITS: "BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)"

# ────────────────────────────────────────────────────────
#  【必须】描述符存放位置
#  0 = DMA_DESC_IN_DEVICE（推荐）
#  1 = DMA_DESC_IN_HOST（测试主机侧 scatter-gather）
# ────────────────────────────────────────────────────────
DEFAULT_DESC_DIRECTION: 0

# ────────────────────────────────────────────────────────
#  【可选】MMU 测试时的寄存器地址（与设备型号相关）
#  切换设备时同步更新
# ────────────────────────────────────────────────────────
MMU_REG_ADDR_WR: "0x202010"  # DMA 写通道 MMU 使能寄存器
MMU_REG_ADDR_RD: "0x202810"  # DMA 读通道 MMU 使能寄存器
MMU_ENABLE_VAL:  "0x10101"   # 写入此值使能 MMU

# ────────────────────────────────────────────────────────
#  【可选】压力测试参数
# ────────────────────────────────────────────────────────
STRESS_REPEAT_CNT:  10       # 压力测试重复次数（非 24h 测试）
STRESS_MAX_DESC_CNT: 500     # 随机链式压力测试最大描述符数

# ────────────────────────────────────────────────────────
#  【可选】BAR 访问测试配置
#  根据实际硬件 BAR 布局调整
# ────────────────────────────────────────────────────────
BAR_SRAM_INDEX:       0      # BAR0 = SRAM（ph_base 系列）
BAR4_SRAM_INDEX:      4      # BAR4 = SRAM（ph1s_base 系列）
BAR2_DDR_INDEX:       2      # BAR2 = DDR 直接访问
SRAM_TEST_OFFSET: "GPU_BAR4_SHARED_SRAM_BASE"  # SRAM 起始偏移宏
SRAM_TEST_SIZE:   "SHARED_SRAM_SANITY_SIZE"    # SRAM 测试字节数
```

---

## 2. 测试框架概述

### 2.1 目录结构

```
test/
├── CMakeLists.txt          # CMake 构建（自动收集 src/*.cc）
├── src/
│   ├── main.cc             # Catch2 入口；在此完成 pcief_init() 全局初始化
│   ├── mthdma.cc           # DMA 裸机与 DMA Engine 测试（主文件）
│   ├── base.cc             # BAR 访问、ROM、SRAM 测试
│   ├── intr.cc             # 中断 / MSI-X 测试
│   ├── stress.cc           # 压力 / 随机化测试
│   ├── test_thr.cc         # 多线程辅助函数实现
│   └── test_thr.h          # 多线程辅助函数声明 + cal_timeout()
└── lib/
    ├── mt_pcie_f.h         # 用户态主 API（每个测试文件必须包含）
    ├── mt_pcie_f.c         # API 实现
    ├── qy_reg.h            # PCIe 寄存器地址宏
    └── simlog.h            # LInfo / LError 日志宏
```

新建测试文件 `test/src/my_test.cc` 会被 CMake 自动纳入编译，无需修改 CMakeLists.txt。

### 2.2 每个测试文件必须包含的头文件

```cpp
#include "catch2/catch.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "simlog.h"          // LInfo()、LError()
#include "mt-emu-drv.h"      // ioctl 命令、枚举、dma_bare_rw 结构体
#include "mt_pcie_f.h"       // 所有用户态 PCIe/DMA API
#include "test_thr.h"        // cal_timeout()、dma_bare_simple_test 辅助函数
```

> ⚠️  **不要在测试用例内部调用 `pcief_init()`**，该函数已在 `main.cc` 的全局钩子中调用。

### 2.3 编译与运行

```bash
# 编译测试套件
cd test && mkdir -p build && cd build
cmake .. && make

# 按标签运行
./test "[mtdma0]"         # 冒烟测试
./test "[mtdma1]"         # 功能测试
./test "[mtdma_mmu]"      # MMU 测试
./test "[base]"           # BAR/ROM 测试
./test "[intr0]"          # 基础中断测试
./test "[stress]"         # 压力测试
./test "[sanity]"         # 健全性测试（跨 PCIe 速度）
./test -h                 # 列出所有标签
```

---

## 3. 核心 API 速查

### 3.1 DMA 传输

```c
/* 执行一次裸机 DMA 传输，返回 0 表示成功 */
int pcief_dma_bare_xfer(
    uint32_t data_direction,   // 数据方向（见下表）
    uint32_t desc_direction,   // DMA_DESC_IN_DEVICE(0) 或 DMA_DESC_IN_HOST(1)
    uint32_t desc_cnt,         // N 个描述符时填 N-1；单描述符填 0
    uint32_t block_cnt,        // 块数量；0=不使用块模式
    uint32_t ch_num,           // 通道号 0-63
    uint64_t sar,              // 源地址
    uint64_t dar,              // 目的地址
    uint32_t size,             // 传输字节数
    uint32_t timeout_ms        // 超时，推荐用 cal_timeout(size)
);
```

| 数据方向枚举 | 值 | 含义 |
|------------|---|------|
| `DMA_MEM_TO_MEM` | 0 | 主机→主机 (H2H) |
| `DMA_MEM_TO_DEV` | 1 | 主机→设备 (H2D，写) |
| `DMA_DEV_TO_MEM` | 2 | 设备→主机 (D2H，读) |
| `DMA_DEV_TO_DEV` | 3 | 设备→设备 (D2D) |

### 3.2 BAR 寄存器访问

```c
uint32_t pcief_greg_u32(uint8_t fun, uint8_t bar, uint64_t address);
void     pcief_sreg_u32(uint8_t fun, uint8_t bar, uint64_t address, uint32_t value);
uint64_t pcief_greg_u64(uint8_t fun, uint8_t bar, uint64_t address);
void     pcief_sreg_u64(uint8_t fun, uint8_t bar, uint64_t address, uint64_t value);

int pcief_read (uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data);
int pcief_write(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data);
```

### 3.3 DMA 缓冲区与地址查询

```c
long     pcief_dmabuf_malloc(uint64_t len);        // 返回物理地址，0 表示失败
void     pcief_dmabuf_free(long addr);
int      pcief_dmabuf_write(uint64_t offset, uint32_t len, void *data);
int      pcief_dmabuf_read (uint64_t offset, uint32_t len, void *data);

void pcief_get_mtdma_info(uint64_t *host_paddr, uint64_t *host_maddr, uint64_t *mtdma_size);
```

### 3.4 BAR 内存访问辅助函数（mt_pcie_f.h 中的 static 函数）

```c
static int mem_rw   (int fun, int bar, uint64_t offset, uint32_t len);       // 写入后回读比较
static int mem_read (int fun, int bar, uint64_t offset, uint32_t len);
static int mem_write(int fun, int bar, uint64_t offset, uint32_t len);
static int rand_rw  (int fun, int bar, uint64_t offset, uint32_t len,
                     uint64_t max_size);                                      // 随机地址/长度压力
```

### 3.5 多线程 DMA 辅助（test_thr.h）

```cpp
/* 超时计算：基准速度 2 MB/s */
static int cal_timeout(uint64_t size);
// 示例：512 KB → 256 ms

/* 为每个通道创建独立线程执行 DMA，自动断言结果 */
void dma_bare_simple_test(
    uint32_t ch_start_num,        // 起始通道号
    uint32_t ch_cnt,              // 并行通道数
    uint32_t data_direction_bits, // BIT(方向) 位掩码，可多个叠加
    uint32_t desc_direction,      // DMA_DESC_IN_DEVICE 或 DMA_DESC_IN_HOST
    uint32_t desc_cnt,            // N-1（链式）或 0（单描述符）
    uint32_t block_cnt,           // 块数量（0=不用块模式）
    uint64_t device_sar,          // 设备侧源地址（多通道时自动递增）
    uint64_t device_dar,          // 设备侧目的地址（多通道时自动递增）
    uint64_t size,                // 每通道总字节数
    int      cnt,                 // 每线程重复次数
    int      offset               // 地址偏移
);
```

> **关键规则：**
> - `desc_cnt` 始终填 **N-1**（N = 描述符总数），这是硬件约定。
> - `block_cnt = 0` 表示不使用块模式（单链或单描述符）。
> - `data_direction_bits` 使用 `BIT()` 位掩码，可在一次调用中验证多个方向。
> - `dma_bare_simple_test` 会自动为每个通道将 `device_sar` 和 `device_dar` 各递增 `size`。

### 3.6 完整 API 列表（`test/lib/mt_pcie_f.h`）

所有对外接口均显式列出，便于人为更新：

```c
/* ── 初始化 / 清理 ─────────────────────────────────────── */
void pcief_init();           /* 打开所有设备节点、mmap BAR 区域 */
void pcief_uninit();         /* 关闭所有文件描述符，释放映射 */

/* ── 设备信息查询 ──────────────────────────────────────── */
struct pcief_bar *pcief_get_barinfo(uint8_t fun, uint8_t bar);
/* 返回 bar_paddr / bar_vaddr / bar_size / bar_map_addr */

int pcief_get_vf__num();     /* 返回已探测到的 VF 数量（注意：双下划线，与源码一致） */
uint64_t pcief_vf_base_addr(int vf); /* 返回第 vf 个 VF 的 DDR 基地址 */
int pcief_mtdma_pf_ch_num(); /* 返回 PF DMA 通道数 */

/* ── BAR 寄存器读写（普通映射 mmap） ─────────────────────── */
int pcief_write  (uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data);
int pcief_write_s(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data, uint32_t type);
int pcief_read   (uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data);
int pcief_read_s (uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data, uint32_t type);

/* ── IO 寄存器读写（ioctl 方式，不依赖 mmap） ─────────────── */
int pcief_io_write(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data);
int pcief_io_read (uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data);

/* ── PCI 配置空间读写 ─────────────────────────────────── */
int pcief_cfg_write(uint8_t fun, uint32_t offset, uint32_t len, void *data);
int pcief_cfg_read (uint8_t fun, uint32_t offset, uint32_t len, void *data);

/* ── 扩展 ROM 读取 ────────────────────────────────────── */
int pcief_read_exp_rom(uint32_t len, void *data);

/* ── DMA 缓冲区管理（主机侧预留内存） ──────────────────── */
long pcief_dmabuf_malloc(uint64_t len);    /* 分配，返回物理地址；失败返回 0 */
void pcief_dmabuf_free  (long addr);
int  pcief_dmabuf_write (uint64_t offset, uint32_t len, void *data);
int  pcief_dmabuf_read  (uint64_t offset, uint32_t len, void *data);
void pcief_get_mtdma_info(uint64_t *pa, uint64_t *ma, uint64_t *size);
/* 获取 DMA 缓冲区：物理地址 / mmap 地址 / 大小 */

/* ── 裸机 DMA 传输（核心接口） ─────────────────────────── */
int pcief_dma_bare_xfer(
    uint32_t data_direction,  /* DMA_MEM_TO_DEV / DMA_DEV_TO_MEM / DMA_DEV_TO_DEV / DMA_MEM_TO_MEM */
    uint32_t desc_direction,  /* DMA_DESC_IN_DEVICE(0) 或 DMA_DESC_IN_HOST(1) */
    uint32_t desc_cnt,        /* N-1；单描述符填 0 */
    uint32_t block_cnt,       /* 块数量；0=不使用块模式 */
    uint32_t ch_num,          /* 通道号 0-63 */
    uint64_t sar,             /* 源地址 */
    uint64_t dar,             /* 目的地址 */
    uint32_t size,            /* 传输字节数 */
    uint32_t timeout_ms       /* 超时，推荐 cal_timeout(size) */
);

/* ── DMA Engine 传输（高层封装） ────────────────────────── */
void *pcief_mtdma_engine_malloc(uint32_t size); /* 分配 Engine 传输缓冲区 */
void  pcief_mtdma_engine_free(void *ptr);
int   pcief_mtdma_engine_start(int fun, struct mtdma_rw *info,
                                void *rw_buf, uint32_t *done);

/* ── 中断管理 ──────────────────────────────────────────── */
int pcief_irq_init(uint8_t fun, uint8_t type, uint8_t test_mode);
/* type: IRQ_DISABLE/IRQ_LEGACY/IRQ_MSI/IRQ_MSIX；test_mode=1 启用测试模式 */

int pcief_dmaisr_set(uint8_t fun, uint8_t dmabare);
/* dmabare=1：将 MSI-X 路由到裸机 DMA ISR */

int pcief_wait_int(uint8_t fun, int irq, uint32_t *done, uint32_t timeout_ms);
/* 阻塞等待指定 IRQ；返回 0 且 *done=1 表示成功 */

int pcief_trig_int(uint8_t fun, int irq, uint32_t *done);
/* 软件触发指定 IRQ（测试用） */

/* ── 电源管理 ──────────────────────────────────────────── */
int pcief_get_power(uint8_t fun, uint32_t *state);
int pcief_suspend(uint8_t fun, uint32_t state);
int pcief_resume(uint8_t fun);

/* ── 目标设备 IPC（SMC / FEC / DSP） ──────────────────── */
int pcief_tgt_sreg_u32     (uint8_t target, uint64_t address, uint32_t val);
int pcief_tgt_post_sreg_u32(uint8_t target, uint64_t address, uint32_t val); /* 无需等待响应 */
int pcief_tgt_greg_u32     (uint8_t target, uint64_t address, uint32_t *val);
int pcief_tgt_wr           (uint8_t target, uint64_t address, void *data, uint32_t size);
int pcief_tgt_rd           (uint8_t target, uint64_t address, void *data, uint32_t size);
int pcief_tgt_wr_rand      (uint8_t target, uint64_t address, uint32_t size);
int pcief_tgt_crc          (uint8_t target, uint64_t address, uint32_t size, uint32_t *val);
int pcief_tgt_dma          (uint8_t target, uint8_t idx, uint8_t ch,
                             uint64_t src, uint64_t dst, uint32_t size);
int pcief_tgt_cache        (uint8_t target, uint32_t op, uint64_t start, uint64_t size);
int pcief_tgt_mtdma_reset  (uint8_t target);
int pcief_tgt_cmd          (uint8_t target, uint32_t *done, uint32_t timeout_ms);

/* target 取值：PCIEF_TGT_DSP=0, PCIEF_TGT_FEC=1, PCIEF_TGT_SMC=2 */

/* ── 实用函数 ──────────────────────────────────────────── */
unsigned int make_crc(unsigned int crc, unsigned char *string, unsigned int size);
long time_get_ms();
int pcief_reg_mode(uint8_t bypass_secure);
```

---

## 4. 测试标签体系

| 标签 | 层级 | 适用范围 |
|------|------|--------|
| `[mtdma0]` | 冒烟 | 单描述符、基础方向，每次改动必跑 |
| `[mtdma1]` | 功能 | 链式、块模式、多种大小、DDR 地址 |
| `[mtdma_mmu]` | 功能 | MMU 使能的 DMA 传输 |
| `[base]` | 基础 | 扩展 ROM 读取验证 |
| `[ph1s_base]` | 基础 | PH1S BAR4 SRAM 和 BAR2 DDR 读写 |
| `[ph_base]` | 基础 | BAR0 SRAM 读写 |
| `[ph1s_stress]` | 压力 | PH1S SRAM/DDR 随机地址/长度压力 |
| `[ph_stress]` | 压力 | BAR0 SRAM 随机化多次迭代压力 |
| `[intr0]` | 基础 | PF0 软中断触发与等待 |
| `[intr1]` | 功能 | 中断源到目标映射验证 |
| `[sanity]` | 健全 | 跨 PCIe 速度等级基础验证 |
| `[stress]` | 长时 | 含 24 小时 DMA 压力测试 |
| `[perf]` | 性能 | 吞吐量测量，不做数据校验 |

---

## 5. 优秀测试用例示例（来自现有测试集）

### 5.1 单描述符冒烟测试（`[mtdma0]`）

来源：`test/src/mthdma.cc` — `sanity_dma_bare_single_s`

```cpp
TEST_CASE("sanity_dma_bare_single_s", "[mtdma0]") {
    LInfo("TEST_CASE sanity_dma_bare_single_s start\n");

    uint32_t test_ch_num              = 0;
    uint32_t test_ch_cnt              = 1;
    uint32_t test_data_direction_bits = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
    uint32_t test_desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t test_desc_cnt            = 0;   // 单描述符
    uint32_t test_block_cnt           = 0;
    uint64_t test_device_sar          = 0x0;
    uint64_t test_device_dar          = 0x0;
    uint64_t test_size                = 512 * 1024;  // 512 KB
    uint32_t test_cnt                 = 1;

    dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits,
                         test_desc_direction, test_desc_cnt, test_block_cnt,
                         test_device_sar, test_device_dar, test_size, test_cnt, 0);

    LInfo("TEST_CASE sanity_dma_bare_single_s done\n");
}
```

### 5.2 单描述符 DDR 功能测试（含 H2H，`[mtdma1]`）

来源：`test/src/mthdma.cc` — `sanity_dma_bare_single_ddr`

```cpp
TEST_CASE("sanity_dma_bare_single_ddr", "[mtdma1]") {
    LInfo("TEST_CASE sanity_dma_bare_single_ddr start\n");

    uint32_t test_ch_num              = 0;
    uint32_t test_ch_cnt              = 1;
    // 同时测试 H2D、D2H、D2D 三个方向
    uint32_t test_data_direction_bits = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)
                                        |BIT(DMA_DEV_TO_DEV);
    uint32_t test_desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t test_desc_cnt            = 0;
    uint32_t test_block_cnt           = 0;
    uint64_t test_device_sar          = 0x0;
    uint64_t test_device_dar          = 0x0;
    uint64_t test_size                = 512 * 1024;
    uint32_t test_cnt                 = 1;

    dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits,
                         test_desc_direction, test_desc_cnt, test_block_cnt,
                         test_device_sar, test_device_dar, test_size, test_cnt, 0);

    // 追加 H2H 方向测试
    test_data_direction_bits = BIT(DMA_MEM_TO_MEM);
    dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits,
                         test_desc_direction, test_desc_cnt, test_block_cnt,
                         test_device_sar, test_device_dar, test_size, test_cnt, 0);

    LInfo("TEST_CASE sanity_dma_bare_single_ddr done\n");
}
```

### 5.3 链式描述符功能测试（`[mtdma1]`）

来源：`test/src/mthdma.cc` — `sanity_dma_bare_chain_ddr`

```cpp
TEST_CASE("sanity_dma_bare_chain_ddr", "[mtdma1]") {
    LInfo("TEST_CASE sanity_dma_bare_chain_ddr start\n");

    const uint32_t N_DESC             = 32;
    uint32_t test_ch_num              = 0;
    uint32_t test_ch_cnt              = 1;
    uint32_t test_data_direction_bits = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)
                                        |BIT(DMA_DEV_TO_DEV);
    uint32_t test_desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t test_desc_cnt            = N_DESC - 1;   // ⚠️ N-1，不是 N
    uint32_t test_block_cnt           = 0;
    uint64_t test_device_sar          = 0x0;
    uint64_t test_device_dar          = 0x0;
    uint64_t test_size                = N_DESC * 4 * 1024;  // 每段 4 KB
    uint32_t test_cnt                 = 1;

    dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits,
                         test_desc_direction, test_desc_cnt, test_block_cnt,
                         test_device_sar, test_device_dar, test_size, test_cnt, 0);

    test_data_direction_bits = BIT(DMA_MEM_TO_MEM);
    dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits,
                         test_desc_direction, test_desc_cnt, test_block_cnt,
                         test_device_sar, test_device_dar, test_size, test_cnt, 0);

    LInfo("TEST_CASE sanity_dma_bare_chain_ddr done\n");
}
```

### 5.4 块模式传输测试（`[mtdma1]`）

来源：`test/src/mthdma.cc` — `sanity_dma_bare_block_ddr`

```cpp
TEST_CASE("sanity_dma_bare_block_ddr", "[mtdma1]") {
    LInfo("TEST_CASE sanity_dma_bare_block_ddr start\n");

    uint32_t test_ch_num              = 0;
    uint32_t test_ch_cnt              = 1;
    uint32_t test_data_direction_bits = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)
                                        |BIT(DMA_DEV_TO_DEV);
    uint32_t test_desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t test_desc_cnt            = 8;    // 每块 8 个描述符（块模式传实际数量）
    uint32_t test_block_cnt           = 32;   // 32 块
    uint64_t test_device_sar          = 0x0;
    uint64_t test_device_dar          = 0x0;
    uint64_t test_size                = test_block_cnt * 1024;  // 每块 1 KB
    uint32_t test_cnt                 = 1;

    dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits,
                         test_desc_direction, test_desc_cnt, test_block_cnt,
                         test_device_sar, test_device_dar, test_size, test_cnt, 0);

    test_data_direction_bits = BIT(DMA_MEM_TO_MEM);
    dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits,
                         test_desc_direction, test_desc_cnt, test_block_cnt,
                         test_device_sar, test_device_dar, test_size, test_cnt, 0);

    LInfo("TEST_CASE sanity_dma_bare_block_ddr done\n");
}
```

### 5.5 MMU 单描述符测试（`[mtdma_mmu]`）

来源：`test/src/mthdma.cc` — `sanity_dma_bare_single_ddr_mmu`

```cpp
TEST_CASE("sanity_dma_bare_single_ddr_mmu", "[mtdma_mmu]") {
    LInfo("TEST_CASE sanity_dma_bare_single_ddr_mmu start\n");

    // 使能 MMU（写通道 + 读通道）
    pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
    pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);

    dma_bare_simple_test(
        0, 1,
        BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM),
        DMA_DESC_IN_DEVICE,
        0, 0,
        0x0, 0x0,
        1 * 1024 * 1024,   // 1 MB
        1, 0
    );

    LInfo("TEST_CASE sanity_dma_bare_single_ddr_mmu done\n");
}
```

### 5.6 MMU 多尺寸功能测试（1B / 4KB / 16MB / 1GB，`[mtdma_mmu]`）

来源：`test/src/mthdma.cc` — `func_dma_bare_single_ddr_mmu`

```cpp
TEST_CASE("func_dma_bare_single_ddr_mmu", "[mtdma_mmu]") {
    LInfo("TEST_CASE func_dma_bare_single_ddr_mmu start\n");

    uint32_t dirs = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);

    uint64_t sizes[] = { 1, 4*1024, 16*1024*1024, 1024ULL*1024*1024 };
    for (auto sz : sizes) {
        pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
        pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
        dma_bare_simple_test(0, 1, dirs, DMA_DESC_IN_DEVICE, 0, 0, 0x0, 0x0, sz, 1, 0);
    }

    LInfo("TEST_CASE func_dma_bare_single_ddr_mmu done\n");
}
```

### 5.7 多通道并行链式测试（`[mtdma_mmu]`）

来源：`test/src/mthdma.cc` — `random_multi_dma_bare_chain_ddr`

```cpp
TEST_CASE("random_multi_dma_bare_chain_ddr", "[mtdma_mmu]") {
    LInfo("TEST_CASE random_multi_dma_bare_chain_ddr start\n");

    const uint32_t N_DESC   = 32;
    const uint32_t N_CH     = 4;
    uint64_t per_ch_size    = 4 * 1024 * N_DESC;   // 每通道 128 KB

    dma_bare_simple_test(
        0, N_CH,
        BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV),
        DMA_DESC_IN_DEVICE,
        N_DESC - 1, 0,                              // N-1 个描述符
        0x0, per_ch_size * N_CH,                    // SAR/DAR 不重叠
        per_ch_size,
        10, 0                                       // 每线程重复 10 次
    );

    LInfo("TEST_CASE random_multi_dma_bare_chain_ddr done\n");
}
```

### 5.8 MMU 链式压力测试（多地址偏移，`[mtdma_mmu]`）

来源：`test/src/mthdma.cc` — `stress_dma_bare_chain_ddr_mmu`

```cpp
TEST_CASE("stress_dma_bare_chain_ddr_mmu", "[mtdma_mmu]") {
    LInfo("TEST_CASE stress_dma_bare_chain_ddr_mmu start\n");

    uint32_t offsets[] = {0, 1, 2, 4, 8, 16, 25, 31};

    for (auto off : offsets) {
        pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
        pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);

        dma_bare_simple_test(
            0, 1,
            BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM),
            DMA_DESC_IN_DEVICE,
            4999, 0,               // 5000 个描述符链
            (uint64_t)off, (uint64_t)off,
            4 * 1024 * 5000,       // 约 20 MB
            1, 0
        );
    }

    LInfo("TEST_CASE stress_dma_bare_chain_ddr_mmu done\n");
}
```

### 5.9 BAR SRAM 读写测试（`[ph1s_base]`）

来源：`test/src/base.cc`

```cpp
TEST_CASE("base_bar4_sram_rw", "[ph1s_base]") {
    LInfo("TEST_CASE base_bar4_sram_rw start\n");
    REQUIRE(0 == mem_rw(F_GPU, 4, GPU_BAR4_SHARED_SRAM_BASE,
                         SHARED_SRAM_SANITY_SIZE + 7));
    LInfo("TEST_CASE base_bar4_sram_rw done\n");
}

TEST_CASE("base_bar2_ddr_rw", "[ph1s_base]") {
    LInfo("TEST_CASE base_bar2_ddr_rw start\n");
    REQUIRE(0 == mem_rw(F_GPU, 2, 0, SHARED_SRAM_SANITY_SIZE));
    LInfo("TEST_CASE base_bar2_ddr_rw done\n");
}
```

### 5.10 SRAM 随机压力测试（`[ph1s_stress]`）

来源：`test/src/stress.cc`

```cpp
TEST_CASE("stress_bar4_sram_random", "[ph1s_stress]") {
    LInfo("TEST_CASE stress_bar4_sram_random start\n");
    for (int i = 0; i < 10; i++) {
        printf("i=%d\n", i);
        REQUIRE(0 == rand_rw(F_GPU, 4, GPU_BAR4_SHARED_SRAM_BASE,
                              DDR_RANDOM_SIZE, SHARED_SRAM_SIZE));
    }
    LInfo("TEST_CASE stress_bar4_sram_random done\n");
}
```

### 5.11 软中断触发测试（`[intr0]`）

来源：`test/src/intr.cc`

```cpp
TEST_CASE("pf0_intr_target", "[intr0]") {
    LInfo("TEST_CASE pf0_intr_target start\n");

    for (uint32_t irq = 0; irq < QY_INT_SRC_MAX_NUM; irq++) {
        uint32_t done = 0;
        pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 0);
        pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 1);
        pcief_wait_int(F_GPU, irq, &done, 5000);
        pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 0);
        REQUIRE(done == 1);
    }

    LInfo("TEST_CASE pf0_intr_target done\n");
}
```

---

## 6. 测试用例生成模板

AI 工具生成新测试用例时，从以下模板中选择合适的类型。

### 模板 A — 单描述符冒烟测试（最简单，优先使用）

```cpp
TEST_CASE("{测试名称}", "[mtdma0]") {
    LInfo("TEST_CASE {测试名称} start\n");

    dma_bare_simple_test(
        0,                                                    // ch_start
        1,                                                    // ch_cnt
        BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM),             // 方向位掩码
        DMA_DESC_IN_DEVICE,                                   // 描述符位置
        0,                                                    // desc_cnt（单描述符）
        0,                                                    // block_cnt
        0x0, 0x0,                                             // SAR, DAR
        {传输字节数},                                          // size
        1, 0                                                  // cnt, offset
    );

    LInfo("TEST_CASE {测试名称} done\n");
}
```

### 模板 B — 链式描述符功能测试

```cpp
TEST_CASE("{测试名称}", "[mtdma1]") {
    LInfo("TEST_CASE {测试名称} start\n");

    const uint32_t N_DESC = {描述符数量};   // 例如 32

    dma_bare_simple_test(
        0, 1,
        BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV),
        DMA_DESC_IN_DEVICE,
        N_DESC - 1,   // ⚠️ 硬件要求填 N-1
        0,
        0x0, 0x0,
        N_DESC * {每段字节数},   // 总大小 = N × 每段大小
        1, 0
    );

    LInfo("TEST_CASE {测试名称} done\n");
}
```

### 模板 C — 块模式传输测试

```cpp
TEST_CASE("{测试名称}", "[mtdma1]") {
    LInfo("TEST_CASE {测试名称} start\n");

    const uint32_t N_BLOCK = {块数};      // 例如 32
    const uint32_t D_DESC  = {每块描述符数};  // 例如 8

    dma_bare_simple_test(
        0, 1,
        BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV),
        DMA_DESC_IN_DEVICE,
        D_DESC,    // 块模式传实际描述符数（非 N-1）
        N_BLOCK,
        0x0, 0x0,
        N_BLOCK * {每块字节数},
        1, 0
    );

    LInfo("TEST_CASE {测试名称} done\n");
}
```

### 模板 D — 带 MMU 的测试

```cpp
TEST_CASE("{测试名称}", "[mtdma_mmu]") {
    LInfo("TEST_CASE {测试名称} start\n");

    // 使能 MMU（地址来自配置项 MMU_REG_ADDR_WR/RD 和 MMU_ENABLE_VAL）
    pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
    pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);

    dma_bare_simple_test(
        0, 1,
        BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM),
        DMA_DESC_IN_DEVICE,
        {desc_cnt}, {block_cnt},
        0x0, 0x0,
        {传输字节数},
        1, 0
    );

    LInfo("TEST_CASE {测试名称} done\n");
}
```

### 模板 E — 多通道并行测试

```cpp
TEST_CASE("{测试名称}", "[mtdma1]") {
    LInfo("TEST_CASE {测试名称} start\n");

    const uint32_t N_CH      = {通道数};   // 例如 4
    const uint64_t CHAN_SIZE  = {每通道字节数};

    // dma_bare_simple_test 自动为每个通道递增 SAR/DAR（各增加 CHAN_SIZE）
    dma_bare_simple_test(
        0, N_CH,
        BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM),
        DMA_DESC_IN_DEVICE,
        0, 0,
        0x0, CHAN_SIZE * N_CH,   // DAR 起点偏移，避免与 SAR 区域重叠
        CHAN_SIZE,
        1, 0
    );

    LInfo("TEST_CASE {测试名称} done\n");
}
```

### 模板 F — 直接调用 `pcief_dma_bare_xfer`（不使用辅助线程）

```cpp
TEST_CASE("{测试名称}", "[mtdma0]") {
    LInfo("TEST_CASE {测试名称} start\n");

    const uint32_t size    = {传输字节数};
    const uint64_t dev_dar = LADDR_MTDMA_TEST;

    int ret = pcief_dma_bare_xfer(
        DMA_MEM_TO_DEV,
        DMA_DESC_IN_DEVICE,
        0, 0, 0,
        0x0, dev_dar,
        size,
        cal_timeout(size)
    );
    REQUIRE(ret == 0);

    LInfo("TEST_CASE {测试名称} done\n");
}
```

### 模板 G — BAR 内存读写测试

```cpp
TEST_CASE("{测试名称}", "[ph1s_base]") {
    LInfo("TEST_CASE {测试名称} start\n");

    REQUIRE(0 == mem_rw(F_GPU, {BAR 编号}, {偏移地址}, {字节数}));

    LInfo("TEST_CASE {测试名称} done\n");
}
```

### 模板 H — 随机化压力测试

```cpp
TEST_CASE("{测试名称}", "[stress]") {
    LInfo("TEST_CASE {测试名称} start\n");

    for (int i = 0; i < {STRESS_REPEAT_CNT}; i++) {
        uint32_t desc_cnt  = (uint32_t)rand() % {STRESS_MAX_DESC_CNT};
        uint64_t size      = ((uint64_t)rand() % (16 * 1024 * 1024)) & ~3ULL;
        if (size == 0) size = 4;
        uint32_t ch        = (uint32_t)rand() % PCIE_DMA_CH_NUM;

        dma_bare_simple_test(
            ch, 1,
            BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM),
            DMA_DESC_IN_DEVICE,
            desc_cnt, 0,
            0x0, 0x0, size,
            1, 0
        );
    }

    LInfo("TEST_CASE {测试名称} done\n");
}
```

---

## 7. AI 辅助测试生成指导

当需要让 AI 生成新测试用例时，在此文件中按以下格式描述需求，
AI 工具会根据上方模板和配置项生成完整代码。

### 7.1 测试需求描述格式

```
# 新测试需求

测试名称：{snake_case 名称，格式：<类型>_<场景>_<变体>}
测试标签：{从第 4 节选择合适标签}
测试类型：{单描述符 / 链式 / 块模式 / MMU / 多通道 / 压力 / BAR 访问}

## 核心参数（未填写则使用第 1 节配置项默认值）
- 通道：ch_start={}, ch_cnt={}
- 传输方向：{H2D / D2H / D2D / H2H，可多选}
- 描述符数量：{}（链式时填实际数量 N，代码中自动换算为 N-1）
- 块数量：{}（0=不使用块模式）
- 传输大小：{} 字节
- MMU：{是/否}，使能寄存器地址={}, 值={}
- 重复次数：{}

## 预期行为
- {}（描述预期的成功/失败场景）

## 特殊逻辑
- {}（如：验证错误中断、地址边界、多次循环等）
```

### 7.2 预置测试需求（可由 AI 直接生成）

以下是根据实际需求预先描述的测试，**AI 工具读取本文件后应直接生成对应代码**，
追加到 `test/src/mthdma.cc` 或新建文件 `test/src/custom_test.cc`。

---

#### 需求 T001 — 全通道顺序扫描（冒烟）

```
测试名称：sanity_dma_bare_all_ch_scan
测试标签：[mtdma0]
测试类型：单描述符

核心参数：
- 通道：ch_start=0，ch_cnt=1（循环遍历 0~63 每个通道）
- 传输方向：H2D、D2H
- 描述符数量：0（单描述符）
- 传输大小：64 KB
- 重复次数：1

预期行为：
- 每个通道均能成功完成 H2D 和 D2H 传输

特殊逻辑：
- 用 for 循环遍历 ch=0..PCIE_DMA_CH_NUM-1，每次调用 dma_bare_simple_test(ch, 1, ...)
```

---

#### 需求 T002 — 链式描述符边界值测试（1/2/最大描述符数）

```
测试名称：func_dma_bare_chain_boundary
测试标签：[mtdma1]
测试类型：链式

核心参数：
- 通道：0，单通道
- 传输方向：H2D、D2H
- 描述符数量：依次测试 1、2、500 个描述符
- 传输大小：描述符数 × 4 KB
- 重复次数：1

预期行为：
- 1、2、500 个描述符链均能成功传输
```

---

#### 需求 T003 — 块模式超大块数测试

```
测试名称：func_dma_bare_block_large
测试标签：[mtdma1]
测试类型：块模式

核心参数：
- 通道：0，单通道
- 传输方向：H2D、D2H、D2D
- 每块描述符数：16
- 块数量：64
- 传输大小：64 × 1 KB = 64 KB
- 重复次数：1

预期行为：
- 64 块 × 16 描述符均能完成传输
```

---

#### 需求 T004 — 主机描述符（DMA_DESC_IN_HOST）功能验证

```
测试名称：func_dma_bare_chain_host_desc
测试标签：[mtdma1]
测试类型：链式

核心参数：
- 通道：0，单通道
- 传输方向：H2D、D2H
- desc_direction：DMA_DESC_IN_HOST（1）
- 描述符数量：16
- 传输大小：16 × 4 KB = 64 KB
- 重复次数：1

预期行为：
- 描述符存放在主机内存时传输成功
```

---

#### 需求 T005 — 4 通道并行链式压力测试

```
测试名称：stress_dma_bare_4ch_chain
测试标签：[stress]
测试类型：多通道 + 链式

核心参数：
- 通道：ch_start=0，ch_cnt=4
- 传输方向：H2D、D2H、D2D
- 描述符数量：31（32 个描述符，填 N-1=31）
- 传输大小：每通道 32 × 4 KB = 128 KB
- 重复次数：100

预期行为：
- 4 个通道并行运行 100 次链式传输均成功
```

---

#### 需求 T006 — VF DMA 单通道冒烟测试

```
测试名称：sanity_vf_dma_bare_single
测试标签：[mtdma0]
测试类型：单描述符

核心参数：
- 使用 VF0（F_VGUP(0)）的 DDR 窗口 LADDR_VGPU(0)
- 传输方向：H2D
- 描述符数量：0（单描述符）
- 传输大小：64 KB
- 重复次数：1

特殊逻辑：
- 直接调用 pcief_dma_bare_xfer()，DAR 设为 LADDR_VGPU(0)
```

---

#### 需求 T007 — MMU 块模式压力（随机偏移）

```
测试名称：stress_dma_bare_block_mmu_random_offset
测试标签：[mtdma_mmu]
测试类型：块模式 + MMU + 压力

核心参数：
- 通道：0，单通道
- 传输方向：H2D、D2H
- 每块描述符数：5（使用随机标志 rand<<31 叠加）
- 块数量：64
- 传输大小：64 × 4 KB × 5 = 1280 KB
- 重复次数：10（在 for 循环中）
- MMU 寄存器：0x202010=0x10101, 0x202810=0x10101

预期行为：
- 10 次块模式 + MMU 传输均成功
```

---

#### 需求 T008 — BAR0 SRAM 随机读写压力测试

```
测试名称：stress_bar0_sram_random_ext
测试标签：[ph_stress]
测试类型：BAR 访问 + 压力

核心参数：
- BAR：0（BAR0）
- 基址：GPU_BAR0_SHARED_SRAM_BASE
- 随机偏移：rand() / 0x40000（字节对齐）
- 随机长度：rand() / 0x1000
- 重复次数：10

预期行为：
- 所有随机地址/长度的读写均一致（mem_rw 返回 0）
```

---

## 8. 编码规范

1. 测试用例名遵循 `<类型>_<场景>_<变体>` 格式，例如 `func_dma_bare_chain_ddr_4KB`。
2. 使用 `REQUIRE(条件)` 做致命断言；`CHECK(条件)` 做非致命断言。
3. 每个测试用例开头和结尾都用 `LInfo("TEST_CASE 名称 start/done\n")` 打印日志。
4. 优先使用 `dma_bare_simple_test` 而非直接调用 `pcief_dma_bare_xfer`。
5. **`desc_cnt` 始终填 N-1**（硬件约定，最常见的填写错误）。
6. 超时始终使用 `cal_timeout(size)` 计算，块模式时乘以 `block_cnt * 10`。
7. 在 `test/src/` 中新建的 `.cc` 文件会被 CMake 自动包含，无需修改 `CMakeLists.txt`。
8. 优先使用 `DMA_DESC_IN_DEVICE`，仅测试主机侧 scatter-gather 时才使用 `DMA_DESC_IN_HOST`。
9. `desc_cnt = 65535` 是特殊哨兵值，触发线程辅助函数使用随机描述符数；此时超时乘以 100。
10. 压力测试中随机化地址时，须将地址限制在 BAR/DDR 窗口范围内。

---

## 9. 常见错误与边界情况

| 现象 | 原因 | 正确做法 |
|------|------|---------|
| 数据校验失败 | `desc_cnt` 填了 N 而不是 N-1 | 始终填 `N_DESC - 1` |
| 块模式总大小不对 | 未算上块数 | `size = N_BLOCK × D_DESC × 每段大小` |
| 传输超时 | 超时太短 | 使用 `cal_timeout(size)` |
| 块模式超时 | 忘记乘以块数 | `cal_timeout(size) * block_cnt * 10` |
| 多通道地址重叠 | 手动计算偏移出错 | 使用 `dma_bare_simple_test`（自动递增） |
| H2H SAR/DAR 设置 | H2H 时两端均为主机缓冲区 | SAR=DAR=0x0（驱动内部处理） |
| ioctl 返回 -ENOENT | 设备节点不存在 | `ls /dev/mt_emu*`，检查驱动是否加载 |
| ioctl 返回 -EBUSY | 通道被占用 | 等待释放或换通道 |
