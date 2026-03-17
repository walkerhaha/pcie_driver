---
title: "Skill Quick Start：仅基于 skills 重建 PCIe & DMA 体系"
version: "1.0"
last_modified: "2026-03-17"
dependencies:
  - "README.md"
  - "environment-skill.zh-CN.md"
  - "driver-skill.zh-CN.md"
  - "test-suite-skill.zh-CN.md"
target_audience: "AI Agent / Project Replicator"
status: "usable"
source_scope: "docs/skills/"
excludes:
  - "examples/"
  - "original source code inheritance"
---

# QUICK START：只基于 skills 文档重建 PCIe & DMA 驱动、环境与测试集

## 1. 使用目标

这份 quick start 面向“其它 AI 工具只参考 skills 文档，不继承当前仓库源代码”的场景。

目标是让消费方按固定顺序完成三件事：

1. 重建可编译的 PCIe & DMA 驱动骨架
2. 重建可切换参数的构建/加载/测试环境
3. 重建可扩展的测试集与参数化测试框架

---

## 2. 固定消费顺序

### Step 1：先建立环境参数对象

来源：`environment-skill.zh-CN.md`

先不要写驱动代码，先整理以下输入：

```yaml
driver_build:
  kerneldir: /lib/modules/<kernel>/build
  vf_num: 0
  dma_resv_mem: 1
  ddr_sz_gb: 32
  rom_enable: 0

test_build:
  build_type: Debug
  c_compiler: gcc
  cxx_compiler: g++

runtime_hw:
  gpu_bdf: "18:00.0"
  apu_bdf: "18:00.1"
  root_port_bdf: "17:00.0"
  pcie_gen: 4
  vf_num: 0
  reserved_mem_start: 0x400000000
  reserved_mem_size: 0x800000000

test_runtime:
  catch_filter: "[mtdma0]"
  irq_mode: "msi"
  irq_test_mode: 1
  timeout_ms: 30000
  pcie_gen: 4
  need_rescan: true
```

结论：**所有可变项先参数化，后写代码。**

---

## 3. Step 2：先搭驱动头文件契约，再写实现

来源：`driver-skill.zh-CN.md`

### 2.1 先定义常量与协议头

优先建立以下头文件级契约：

- 设备 ID
- 设备节点名
- DDR / 本地地址布局
- IOCTL 编号
- DMA 方向枚举
- `struct mt_emu_param`
- `struct mtdma_rw`
- `struct dma_bare_rw`
- `struct emu_pcie`
- `struct dma_bare_ch`
- `struct dma_bare`
- `struct dma_ch_desc`

### 2.2 再搭驱动 5 层

按以下顺序实现，而不是并行乱写：

1. PCIe 设备前端层
2. 公共运行时状态层
3. 用户态接口桥接层
4. bare DMA 执行层
5. dmabuf / DMA engine 层

### 2.3 最小闭环优先

先达到这个最小闭环：

1. `probe()` 能映射 BAR
2. 能注册 `/dev/mt_emu_gpu`
3. 能注册 `/dev/mt_emu_dmabuf`
4. `mmap + ioctl` 可用
5. `MT_IOCTL_MTDMA_BARE_RW` 能发起一次单描述符 DMA
6. DMA 中断能回收完成态

不要一开始就追求：

- 完整 DMA engine
- 全量 VF
- 全量 AER / IPC / shell

---

## 4. Step 3：实现用户态发现与测试 API

来源：`test-suite-skill.zh-CN.md`

### 3.1 先重建设备发现层

测试框架应通过以下契约发现驱动，而不是硬编码 BAR：

- `/dev/mt_emu_gpu`
- `/dev/mt_emu_dmabuf`
- `/dev/mt_emu_vgpu<N>`
- `/sys/class/misc/<device>/bar0`
- `/sys/class/misc/<device>/bar2`
- `/sys/class/misc/mt_emu_gpu/vf`

### 3.2 再重建 `pcief_*` API

至少先实现这些公开接口：

```c
void pcief_init();
void pcief_uninit();
int pcief_dma_bare_xfer(...);
uint32_t pcief_greg_u32(...);
void pcief_sreg_u32(...);
int pcief_irq_init(...);
int pcief_wait_int(...);
int pcief_trig_int(...);
long pcief_dmabuf_malloc(uint64_t len);
void pcief_dmabuf_free(long addr);
```

### 3.3 最先恢复的测试 helper

```c++
static int cal_timeout(uint64_t size);
static void dma_bare_simple_test(...);
int start_thr_rand_dma_bare(...);
```

---

## 5. Step 4：按测试分类分批恢复

推荐顺序：

### 第一批：冒烟

- `[mtdma0]`
- `[base]` 中最简单的 BAR/寄存器访问

### 第二批：核心功能

- `[mtdma1]`
- `[intr]`

### 第三批：进阶功能

- `[mtdma_mmu]`
- `[stress]`

### 第四批：补充能力

- DMA engine 用例
- IPC / AER / shell

---

## 6. 推荐的落地文件骨架

未来项目可沿用这个“知识结构”，但不继承本仓库实现：

```text
include/
  driver_protocol.h
  dma_layout.h
  dma_bare.h
  dma_engine.h

driver/
  pcie_frontend_gpu.c
  pcie_frontend_apu.c
  pcie_frontend_vf.c
  ioctl_bridge.c
  dma_bare.c
  dma_dmabuf.c
  dma_engine.c

test/
  lib/pcief_api.c
  src/main.cc
  src/base.cc
  src/intr.cc
  src/mtdma.cc
  src/stress.cc
```

---

## 7. AI 消费时的硬性规则

1. 只继承 skills 中定义的契约和流程，不继承原始源代码。
2. 遇到同名接口时，以 skill 中列出的参数与职责为准。
3. 任何新增参数必须先补到 environment skill。
4. 任何新增测试标签、关键 API、IOCTL 必须先补到对应 skill。
5. 默认排除 `examples/`，除非人类显式要求把它们当参考资料。

---

## 8. 最短执行清单

- [ ] 先完成环境参数对象
- [ ] 先完成驱动协议头与结构体
- [ ] 先完成 PF0 + dmabuf + bare DMA 最小闭环
- [ ] 先完成 `pcief_init()` 和 `pcief_dma_bare_xfer()`
- [ ] 先恢复 `[mtdma0]` 冒烟测试
- [ ] 再逐步恢复 `[mtdma1]`、`[intr]`、`[stress]`

如果只想让 AI 工具快速起步，就按这份 quick start 执行，不必一次性读完整个三份主 skill。
