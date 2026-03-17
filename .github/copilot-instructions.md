# MT EMU PCIe DMA Driver — AI Coding Skill

> This file teaches GitHub Copilot (and other AI tools) how to write test cases for the
> MT EMU PCIe DMA kernel driver.  Everything below is derived from the **non-examples**
> source files in this repository: `driver/`, `test/`, and `scripts/`.

---

## 1. Project Overview

**MT EMU PCIe DMA Driver** is a Linux kernel + user-space test suite for a
Synopsys DesignWare-based PCIe DMA controller used in MT/Mthreads GPU/APU devices.

| Item | Value |
|------|-------|
| Vendor ID | `0x1ed5` |
| GPU device IDs | `0x0200` (QY), `0x0400` (QY2), `0x0500` (PH1S), `0x0680` (HS), `0x0610` (LS) |
| DMA write channels | 64 per PF |
| DMA read channels  | 64 per PF |
| Max virtual functions | 60 |
| Default DDR size | 48 GB (configurable via `DDR_SZ_GB`) |

Key kernel modules produced by `driver/Makefile`:

```
mt_emu_gpu.ko   – PF0 (GPU) driver (file ops, BAR mapping, ioctl, DMA bare, interrupts)
mt_emu_apu.ko   – PF1 (APU) driver
mt_emu_vgpu.ko  – Virtual function driver (up to 60 VFs)
mt_emu_mtdma.ko – Linux DMA Engine wrapper
```

---

## 2. Test Framework

All user-space tests use **Catch2** (header-only C++11) located in `test/src/catch2/`.

### File layout

```
test/
├── CMakeLists.txt          # Build system
├── src/
│   ├── main.cc             # Catch2 session entry point
│   ├── base.cc             # BAR access, ROM, SRAM tests
│   ├── mthdma.cc           # DMA bare-metal tests  ← main test file
│   ├── intr.cc             # Interrupt / MSI-X tests
│   ├── stress.cc           # Stress / random tests
│   └── test_thr.cc         # Thread helpers (implementation)
│   └── test_thr.h          # Thread helper declarations + cal_timeout()
└── lib/
    ├── mt_pcie_f.h         # Primary user-space API header  ← ALWAYS include
    ├── mt_pcie_f.c         # API implementation
    ├── qy_reg.h            # PCIe register base addresses
    └── simlog.h            # LInfo / LError logging macros
```

### Required includes for every test file

```cpp
#include "catch2/catch.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "simlog.h"          // LInfo(), LError()
#include "mt-emu-drv.h"      // ioctl numbers, enums, dma_bare_rw
#include "mt_pcie_f.h"       // ALL user-space PCIe/DMA API functions
#include "test_thr.h"        // cal_timeout(), dma_bare_simple_test helpers
```

### Build & run

```bash
# Build driver modules
cd driver && make

# Build tests
cd test && mkdir -p build && cd build
cmake .. && make

# Run specific tag
./test -t "[mtdma0]"      # sanity
./test -t "[mtdma1]"      # functional DMA
./test -t "[mtdma_mmu]"   # MMU+DMA
./test -t "[base]"        # BAR/ROM
./test -h                 # list all tags
```

---

## 3. Device Function Constants

Defined in `test/lib/mt_pcie_f.h`:

```c
#define F_GPU      0    // PF0 — primary GPU function
#define F_APU      1    // PF1 — APU function
#define F_VGUP(i)  (2 + i)  // VF i  (i = 0..VF_NUM-1)
#define F_MTDMA    (VF_NUM + 2)   // DMA buffer device
```

---

## 4. Core DMA API

### `pcief_dma_bare_xfer` — perform one DMA transfer

```c
int pcief_dma_bare_xfer(
    uint32_t data_direction,   // direction enum (see §5)
    uint32_t desc_direction,   // DMA_DESC_IN_DEVICE (0) or DMA_DESC_IN_HOST (1)
    uint32_t desc_cnt,         // number of descriptors MINUS ONE  (0 = single)
    uint32_t block_cnt,        // number of blocks (0 = no block mode)
    uint32_t ch_num,           // DMA channel index 0–63
    uint64_t sar,              // source address
    uint64_t dar,              // destination address
    uint32_t size,             // transfer size in bytes
    uint32_t timeout_ms        // use cal_timeout(size) for standard timeout
);
// Returns 0 on success, non-zero on error.
```

### `pcief_read` / `pcief_write` — BAR register access

```c
int pcief_read (uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data);
int pcief_write(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void *data);

// Convenience typed wrappers (inline in mt_pcie_f.h):
uint32_t pcief_greg_u32(uint8_t fun, uint8_t bar, uint64_t address);
void     pcief_sreg_u32(uint8_t fun, uint8_t bar, uint64_t address, uint32_t value);
uint64_t pcief_greg_u64(uint8_t fun, uint8_t bar, uint64_t address);
void     pcief_sreg_u64(uint8_t fun, uint8_t bar, uint64_t address, uint64_t value);
```

### DMA buffer allocation (host-side reserved memory)

```c
long pcief_dmabuf_malloc(uint64_t len);      // returns physical address or 0 on error
void pcief_dmabuf_free(long addr);
int  pcief_dmabuf_write(uint64_t offset, uint32_t len, void *data);
int  pcief_dmabuf_read (uint64_t offset, uint32_t len, void *data);
```

### Interrupt helpers

```c
int pcief_wait_int(uint8_t fun, int irq, uint32_t *done, uint32_t timeout_ms);
int pcief_trig_int(uint8_t fun, int irq, uint32_t *done);
int pcief_dmaisr_set(uint8_t fun, uint8_t dmabare);
int pcief_irq_init(uint8_t fun, uint8_t type, uint8_t test_mode);
```

---

## 5. Data Direction Enums

```c
// Defined in driver/mt-emu-drv.h (user-space guard: #ifndef __KERNEL__)
enum dma_transfer_direction {
    DMA_MEM_TO_MEM,   // 0 – Host-to-Host  (H2H): both SAR and DAR in host RAM
    DMA_MEM_TO_DEV,   // 1 – Host-to-Device (H2D): SAR = host, DAR = device DDR
    DMA_DEV_TO_MEM,   // 2 – Device-to-Host (D2H): SAR = device DDR, DAR = host
    DMA_DEV_TO_DEV,   // 3 – Device-to-Device (D2D): both in device DDR
    DMA_TRANS_NONE,
};

// Descriptor placement
#define DMA_DESC_IN_DEVICE  0   // Descriptors stored in device DDR (optimal)
#define DMA_DESC_IN_HOST    1   // Descriptors stored in host RAM
```

### How to specify multiple directions in one test

Use a bitmask: `BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM)`.
`dma_bare_simple_test()` iterates over all set bits.

```c
uint32_t dirs = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM) | BIT(DMA_DEV_TO_DEV);
```

---

## 6. Transfer Modes

### 6.1 Single-descriptor transfer

```
desc_cnt  = 0
block_cnt = 0
```
Hardware programs SAR/DAR/ACNT directly into channel registers; one interrupt on done.

### 6.2 Chained-descriptor transfer

```
desc_cnt  = N - 1    (N = number of descriptors, e.g. 31 for 32 descriptors)
block_cnt = 0
```
Each descriptor's `lar` field points to the next descriptor in device DDR.
The last descriptor has `lar = 0` and `desc_op |= DMA_CH_DESC_BIT_INTR_EN`.
Total transfer size = N × (size / N) — pass the total byte count as `size`.

### 6.3 Block transfer

```
desc_cnt  = D - 1    (D descriptors per block)
block_cnt = B        (B ≥ 1)
```
Repeats D-descriptor chain B times.
Total size = B × D × segment_size — pass total as `size`.

---

## 7. Key Helper: `dma_bare_simple_test`

Defined in `test/src/mthdma.cc`, used by all DMA test cases:

```cpp
static void dma_bare_simple_test(
    uint32_t ch_start_num,        // first channel index
    uint32_t ch_cnt,              // number of parallel channels
    uint32_t data_direction_bits, // bitmask of DMA directions to test
    uint32_t desc_direction,      // DMA_DESC_IN_DEVICE or DMA_DESC_IN_HOST
    uint32_t desc_cnt,            // N-1 descriptors (0 = single)
    uint32_t block_cnt,           // blocks (0 = no block mode)
    uint64_t device_sar,          // starting device source address
    uint64_t device_dar,          // starting device dest address
    uint64_t size,                // total bytes per channel
    int      cnt,                 // repeat count per thread
    int      offset               // address offset
);
```

This function spawns `ch_cnt` threads (one per channel), runs the transfer, and
uses `REQUIRE(*ret == 0)` to assert success.

### Timeout calculation

```cpp
#define MTDMA_EDK_SPEED  (2*1024*1024)   // 2 MB/s baseline

static int cal_timeout(uint64_t size) {
    return ((size * 1000) / MTDMA_EDK_SPEED);
}
// Example: 512KB → 256 ms
```

---

## 8. BAR / Memory Access Helpers

Defined as static functions in `test/lib/mt_pcie_f.h`:

```cpp
// Write then read back, compare
static int mem_rw(int fun, int bar, uint64_t offset, uint32_t len);

// Read-only
static int mem_read(int fun, int bar, uint64_t offset, uint32_t len);

// Write-only
static int mem_write(int fun, int bar, uint64_t offset, uint32_t len);

// Random address/length read-write stress (8 alignments × 4 widths)
static int rand_rw(int fun, int bar, uint64_t offset, uint32_t len,
                   uint64_t max_size);
```

---

## 9. Test Tag Conventions

| Tag | Scope |
|-----|-------|
| `[mtdma0]` | Sanity / smoke — single descriptor, basic directions |
| `[mtdma1]` | Functional — chain, block, various sizes, DDR |
| `[mtdma_mmu]` | MMU-enabled DMA transfers |
| `[base]` | Expansion ROM read verification |
| `[ph1s_base]` | PH1S BAR4 SRAM and BAR2 DDR R/W |
| `[ph1s_stress]` | PH1S SRAM/DDR stress (random address, random length) |
| `[ph_base]` | General BAR0 SRAM access |
| `[ph_stress]` | Randomized multi-iteration SRAM stress |
| `[intr0]` | Basic interrupt routing (PF0 soft trigger) |
| `[intr1]` | Advanced interrupt routing (src-to-target mapping) |

---

## 10. Device DDR Address Map

```c
// From driver/mt-emu-drv.h
#define DDR_SZ_GB          48             // default
#define DDR_SZ             (0x40000000ULL * DDR_SZ_GB)   // 48 GB
#define DDR_SZ_RESV        0x0040000000ULL               // 1 GB reserved
#define DDR_SZ_FREE        (DDR_SZ - DDR_SZ_RESV)        // usable top

#define LADDR_APU          (DDR_SZ_FREE)
#define LADDR_MTDMA_LL_WR  (DDR_SZ_FREE + 0x2000000)    // DMA WR link-list
#define LADDR_MTDMA_LL_RD  (LADDR_MTDMA_LL_WR + 0x10000)
#define LADDR_MTDMA_TEST    0x100000                     // standard test region

// VF DDR windows (up to 60 VFs, 1 GB each)
#define SIZE_VGPU_DDR       0x40000000ULL
#define LADDR_VGPU(vf)      (LADDR_VGPU_BASE + SIZE_VGPU_DDR * (vf))
```

---

## 11. DMA Register Reference

### Common controller registers (base `REG_DMA_COMM_BASE = 0x380000`)

| Offset | Name | Description |
|--------|------|-------------|
| 0x000  | BASIC_PARAM  | Version (read-only) |
| 0x010  | COMM_ENABLE  | Enable DMA controller |
| 0x400  | CH_NUM       | Number of channels – 1 |
| 0x408  | MST0_BLEN    | Master 0 burst length |
| 0xC00  | ALARM_IMSK   | Alarm interrupt mask |
| 0xC04  | ALARM_RAW    | Raw alarm status |
| 0xD00  | WORK_STS     | DMA busy status |

### Per-channel registers (base `REG_DMA_CHAN_BASE = 0x383000`, stride 0x1000/channel)

| Offset | Name | Description |
|--------|------|-------------|
| 0x000  | ENABLE      | Bit 0: start transfer |
| 0x004  | DIRECTION   | Address-type + cross/local flags |
| 0x010  | MMU_ADDR_TYPE | MMU translation mode |
| 0x0C4  | INTR_IMSK   | Interrupt mask |
| 0x0C8  | INTR_RAW    | Raw interrupt (write-1-to-clear) |
| 0x0CC  | INTR_STATUS | Masked interrupt |
| 0x0D0  | STATUS      | Bit 0: busy |
| 0x400  | DESC_OPT    | Bit 0: intr_en, Bit 1: chain_en |
| 0x404  | ACNT        | Byte count |
| 0x408  | SAR_L       | Source address [31:0] |
| 0x40C  | SAR_H       | Source address [63:32] |
| 0x410  | DAR_L       | Destination address [31:0] |
| 0x414  | DAR_H       | Destination address [63:32] |
| 0x418  | LAR_L       | Link-list address [31:0] |
| 0x41C  | LAR_H       | Link-list address [63:32] |

### Interrupt bits (`REG_DMA_CH_INTR_RAW`)

```c
#define DMA_CH_INTR_BIT_DONE            BIT(0)  // success
#define DMA_CH_INTR_BIT_ERR_DATA        BIT(1)  // data error
#define DMA_CH_INTR_BIT_ERR_DESC_READ   BIT(2)  // cannot read descriptor
#define DMA_CH_INTR_BIT_ERR_CFG         BIT(3)  // configuration error
#define DMA_CH_INTR_BIT_ERR_DUMMY_READ  BIT(4)  // dummy-read (addr check) failed
```

---

## 12. `dma_ch_desc` Structure (28 bytes, packed)

```c
// driver/mt-emu-mtdma-bare.h
struct dma_ch_desc {
    uint32_t desc_op;   // BIT(0)=intr_en, BIT(1)=chain_en
    uint32_t cnt;       // byte count for this segment
    union { uint64_t reg; struct { uint32_t lsb, msb; }; } sar;  // source addr
    union { uint64_t reg; struct { uint32_t lsb, msb; }; } dar;  // dest addr
    union { uint64_t reg; struct { uint32_t lsb, msb; }; } lar;  // next desc addr
} __packed;
```

---

## 13. Test Case Templates

### Template A — Single-descriptor smoke test

```cpp
TEST_CASE("my_dma_single_h2d", "[mtdma0]") {
    LInfo("TEST_CASE my_dma_single_h2d start\n");

    uint32_t ch_num              = 0;
    uint32_t ch_cnt              = 1;
    uint32_t data_direction_bits = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM);
    uint32_t desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t desc_cnt            = 0;   // single descriptor
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

### Template B — Chained-descriptor test

```cpp
TEST_CASE("my_dma_chain_32desc", "[mtdma1]") {
    LInfo("TEST_CASE my_dma_chain_32desc start\n");

    const uint32_t N_DESC        = 32;
    uint32_t ch_num              = 0;
    uint32_t ch_cnt              = 1;
    uint32_t data_direction_bits = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM)
                                   | BIT(DMA_DEV_TO_DEV);
    uint32_t desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t desc_cnt            = N_DESC - 1;  // N-1
    uint32_t block_cnt           = 0;
    uint64_t device_sar          = 0x0;
    uint64_t device_dar          = 0x0;
    uint64_t size                = N_DESC * 4 * 1024;  // 4 KB per descriptor
    int      cnt                 = 1;

    dma_bare_simple_test(ch_num, ch_cnt, data_direction_bits,
                         desc_direction, desc_cnt, block_cnt,
                         device_sar, device_dar, size, cnt, 0);

    LInfo("TEST_CASE my_dma_chain_32desc done\n");
}
```

### Template C — Block-transfer test

```cpp
TEST_CASE("my_dma_block_32x8", "[mtdma1]") {
    LInfo("TEST_CASE my_dma_block_32x8 start\n");

    uint32_t ch_num              = 0;
    uint32_t ch_cnt              = 1;
    uint32_t data_direction_bits = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM)
                                   | BIT(DMA_DEV_TO_DEV);
    uint32_t desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t block_cnt           = 32;
    uint32_t desc_cnt            = 8;   // 8 descriptors per block (bit-31=0: fixed)
    uint64_t device_sar          = 0x0;
    uint64_t device_dar          = 0x0;
    uint64_t size                = block_cnt * 1024;  // 1 KB per block
    int      cnt                 = 1;

    dma_bare_simple_test(ch_num, ch_cnt, data_direction_bits,
                         desc_direction, desc_cnt, block_cnt,
                         device_sar, device_dar, size, cnt, 0);

    LInfo("TEST_CASE my_dma_block_32x8 done\n");
}
```

### Template D — Multi-channel parallel test

```cpp
TEST_CASE("my_dma_multi_channel", "[mtdma1]") {
    LInfo("TEST_CASE my_dma_multi_channel start\n");

    uint32_t ch_start            = 0;
    uint32_t ch_cnt              = 4;   // 4 channels simultaneously
    uint32_t data_direction_bits = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM);
    uint32_t desc_direction      = DMA_DESC_IN_DEVICE;
    uint32_t desc_cnt            = 0;
    uint32_t block_cnt           = 0;
    uint64_t device_sar          = 0x0;
    uint64_t device_dar          = 0x0;
    uint64_t size                = 64 * 1024;  // 64 KB per channel
    int      cnt                 = 1;

    // Each channel gets its own address window (device_sar increments by size)
    dma_bare_simple_test(ch_start, ch_cnt, data_direction_bits,
                         desc_direction, desc_cnt, block_cnt,
                         device_sar, device_dar, size, cnt, 0);

    LInfo("TEST_CASE my_dma_multi_channel done\n");
}
```

### Template E — BAR register read/write test

```cpp
TEST_CASE("my_bar0_rw", "[base]") {
    LInfo("TEST_CASE my_bar0_rw start\n");

    // Write + read-back 4 KB starting at BAR0 offset 0
    REQUIRE(0 == mem_rw(F_GPU, 0, 0, 4096));

    LInfo("TEST_CASE my_bar0_rw done\n");
}
```

### Template F — Direct `pcief_dma_bare_xfer` (no helper thread)

```cpp
TEST_CASE("my_single_h2d_direct", "[mtdma0]") {
    LInfo("TEST_CASE my_single_h2d_direct start\n");

    uint64_t dev_addr = LADDR_MTDMA_TEST;
    uint32_t size     = 256 * 1024;   // 256 KB

    int ret = pcief_dma_bare_xfer(
        DMA_MEM_TO_DEV,        // direction
        DMA_DESC_IN_DEVICE,    // desc placement
        0,                     // desc_cnt (single)
        0,                     // block_cnt
        0,                     // channel 0
        0x0,                   // SAR: host DMA buffer base
        dev_addr,              // DAR: device DDR
        size,
        cal_timeout(size)
    );
    REQUIRE(ret == 0);

    LInfo("TEST_CASE my_single_h2d_direct done\n");
}
```

---

## 14. Standard Transfer Sizes

The existing tests cover these sizes; when adding new tests prefer these values:

| Size | Bytes |
|------|-------|
| 1 B   | 1 |
| 512 B | 512 |
| 4 KB  | 4 × 1024 |
| 64 KB | 64 × 1024 |
| 512 KB | 512 × 1024 |
| 16 MB | 16 × 1024 × 1024 |
| 1 GB  | 1024 × 1024 × 1024 |

---

## 15. Interrupt / IRQ Test Pattern

```cpp
TEST_CASE("my_pf0_soft_intr", "[intr0]") {
    LInfo("TEST_CASE my_pf0_soft_intr start\n");

    uint32_t done = 0;
    uint32_t irq  = QY_INT_SRC_SGI_PF0_TEST;  // soft interrupt source

    // Trigger
    pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 0);
    pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 1);

    // Wait (5 s timeout)
    pcief_wait_int(F_GPU, irq, &done, 5000);
    pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 0);

    REQUIRE(done == 1);
    LInfo("TEST_CASE my_pf0_soft_intr done\n");
}
```

---

## 16. Coding Conventions

1. **Always call `pcief_init()` before any PCIe access** (done globally via Catch2 session fixture in `main.cc`).
2. Use `LInfo()` / `LError()` from `simlog.h` for log output, not `printf` (though `printf` is also used in some files).
3. Test case names follow the pattern `<scope>_<feature>_<variant>`.
4. Use `REQUIRE(cond)` for fatal assertions; `CHECK(cond)` for non-fatal.
5. `dma_bare_simple_test` is preferred over calling `pcief_dma_bare_xfer` directly — it handles threading, timeout computation, and assertion automatically.
6. Always pass `cal_timeout(size)` as the `timeout_ms` argument to avoid spurious timeouts.
7. For `data_direction_bits`, use the `BIT()` macro bitmask to run multiple directions in one test.
8. Descriptor count is always **N−1** for N descriptors (hardware convention).
9. Block count `0` means **no block mode** (single chain or single descriptor).
10. Channel 0 is always safe for single-channel tests; use `ch_start=0, ch_cnt=N` for multi-channel tests to avoid conflict.
11. New test `.cc` files placed in `test/src/` are automatically picked up by CMake (`file(GLOB TEST_SOURCES src/*.cc)`).
12. `DMA_DESC_IN_DEVICE` (0) is preferred for performance; use `DMA_DESC_IN_HOST` (1) only when testing host-side scatter-gather.
13. For block-mode timeout, multiply by `block_cnt`: `cal_timeout(size) * block_cnt * 10`.
14. `desc_cnt = 65535` is a special sentinel that signals the thread helper to use random descriptor counts; multiply timeout by 100 in that case.
15. Multi-channel tests must not overlap device addresses: `dma_bare_simple_test` increments `st_addr` and `ed_addr` by `size` per channel automatically.
16. Use `LADDR_MTDMA_TEST` (0x100000) as the default device DDR start address in new DMA tests.
17. All BAR helper functions (`mem_rw`, `rand_rw`, etc.) are `static` inlines in `mt_pcie_f.h` and can be called directly without any extra setup.
18. Stress tests that randomise address and length must clamp to the BAR/DDR window size to avoid out-of-bounds accesses.

---

## 17. Stress / Randomisation Patterns

```cpp
// Random descriptor count (up to 500)
uint32_t desc_cnt = (uint32_t)rand() % 500;

// Random size aligned to 4 bytes
uint64_t size = ((uint64_t)rand() % (16 * 1024 * 1024)) & ~3ULL;
if (size == 0) size = 4;

// Random channel
uint32_t ch = (uint32_t)rand() % PCIE_DMA_CH_NUM;

// Use block_cnt=0 for standard chain, or non-zero for block mode
uint32_t block_cnt = (uint32_t)rand() % 32;
```

---

## 18. Common Pitfalls & Edge Cases

| Pitfall | How to avoid |
|---------|-------------|
| Timeout too short | Always use `cal_timeout(size)` or multiply by iteration count |
| `desc_cnt` off-by-one | Remember: `desc_cnt = N - 1`, not N |
| Wrong direction for D2D | D2D uses device DDR for both SAR and DAR; pass device addresses for both |
| H2H SAR/DAR both 0 | For H2H, both point to host DMA buffer base (0x0 in the test fixture) |
| Channel address overlap | When running multiple channels, `dma_bare_simple_test` increments `st_addr` by `size` per channel automatically |
| block_cnt × desc_cnt mismatch | Total size = block_cnt × (desc_cnt+1) × segment_size |
| Missing `pcief_init()` | Segfault — init is in `main.cc`; don't call it again in individual tests |
| desc_cnt=65535 (special value) | Signals random descriptor count to the thread helper; multiply timeout by 100 |
