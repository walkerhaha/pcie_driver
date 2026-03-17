---
title: "Skill：源码位置到文档的映射索引"
version: "1.0"
last_modified: "2026-03-17"
dependencies:
  - "driver-skill.zh-CN.md"
  - "environment-skill.zh-CN.md"
  - "test-suite-skill.zh-CN.md"
target_audience: "AI Agent / Project Replicator"
status: "usable"
source_scope:
  - "driver/"
  - "test/"
  - "scripts/"
excludes:
  - "examples/"
---

# Skill：源码位置到文档的映射索引

## 1. 目标

这份索引不定义新的契约，只负责回答一个问题：

> “如果我在源码里看到了某个文件 / 接口 / 数据结构，应该回哪份 skill 的哪一段理解它？”

---

## 2. 驱动文件 -> skill 章节

| 源码文件 | 主要职责 | 优先查阅 |
|---|---|---|
| `driver/mt-emu.h` | 运行时共享结构、PCIe 状态、DMA 子结构 | `driver-skill.zh-CN.md` §4 |
| `driver/mt-emu-drv.h` | 设备名、DDR 布局、IOCTL 号、传输参数结构 | `driver-skill.zh-CN.md` §4 ~ §5 |
| `driver/mt-emu-gpu.c` | PF0 前端驱动、BAR 映射、misc 注册、中断初始化 | `driver-skill.zh-CN.md` §2 / §6 / §8 |
| `driver/mt-emu-apu.c` | PF1 前端驱动 | `driver-skill.zh-CN.md` §2 / §8 |
| `driver/mt-emu-vgpu.c` | VF 前端驱动 | `driver-skill.zh-CN.md` §2 / §8 |
| `driver/mt-emu-ioctl.c` | 用户态 ioctl/read/write/mmap 分发 | `driver-skill.zh-CN.md` §3 / §7 |
| `driver/mt-emu-ioctl.h` | ioctl 桥接声明 | `driver-skill.zh-CN.md` §7 |
| `driver/mt-emu-mtdma-bare.h` | bare DMA 寄存器、方向、描述符声明 | `driver-skill.zh-CN.md` §4 / §5 / §9 |
| `driver/mt-emu-mtdma-bare.c` | bare DMA 提交、同步、中断结果 | `driver-skill.zh-CN.md` §9 ~ §10 |
| `driver/mt-emu-mtdma-core.c` | DMA engine 相关核心逻辑 | `driver-skill.zh-CN.md` §2 / §9 |
| `driver/mt-emu-dmabuf.c` | dmabuf 设备与大缓冲导出 | `driver-skill.zh-CN.md` §2 / §6 |
| `driver/Makefile` | 驱动构建参数与模块构建入口 | `environment-skill.zh-CN.md` §2 ~ §4 |

---

## 3. 测试文件 -> skill 章节

| 源码文件 | 主要职责 | 优先查阅 |
|---|---|---|
| `test/src/main.cc` | Catch2 入口与全局 init/uninit | `test-suite-skill.zh-CN.md` §3 |
| `test/src/test_thr.h` | 线程化参数结构与 helper 声明 | `test-suite-skill.zh-CN.md` §5 |
| `test/lib/mt_pcie_f.h` | 用户态公开 API 总表 | `test-suite-skill.zh-CN.md` §4 |
| `test/lib/mt_pcie_f.c` | API 实现、设备发现、target 命令、DMA helper | `test-suite-skill.zh-CN.md` §4 / `environment-skill.zh-CN.md` §5 |
| `test/src/base.cc` | BAR / CFG / ROM / IPC 基础测试 | `test-suite-skill.zh-CN.md` §8 ~ §10 |
| `test/src/intr.cc` | 中断触发与等待测试 | `test-suite-skill.zh-CN.md` §4.4 / §8 ~ §10 |
| `test/src/mthdma.cc` | bare DMA / DMA engine / reset / 性能 / MMU 测试 | `test-suite-skill.zh-CN.md` §4.5 ~ §4.9 / §6 ~ §10 |
| `test/src/stress.cc` | 稳定性与压力测试 | `test-suite-skill.zh-CN.md` §8 ~ §10 |
| `test/shell/main.c` | 人工调试命令入口，消费 power/target/io 寄存器接口 | `test-suite-skill.zh-CN.md` §4.7 ~ §4.9 |
| `test/CMakeLists.txt` | 测试编译配置 | `environment-skill.zh-CN.md` §2 / §3 / §4 / §8 |

---

## 4. 脚本 -> skill 章节

| 文件 | 主要职责 | 优先查阅 |
|---|---|---|
| `scripts/bulid.sh` | 安装依赖、触发构建 | `environment-skill.zh-CN.md` §2 / §4 |
| `scripts/install.sh` | 卸载/加载模块、打开动态调试 | `environment-skill.zh-CN.md` §2.4 / §4.4 / §6 |
| `scripts/test_sanity.sh` | 链路切速、重扫、测试前准备 | `environment-skill.zh-CN.md` §3.3 / §6 / §7 |

---

## 5. 公开 API -> skill 章节

| API 族 | 典型接口 | 优先查阅 |
|---|---|---|
| 初始化与发现 | `pcief_init()` / `pcief_get_barinfo()` | `test-suite-skill.zh-CN.md` §4.1 / `environment-skill.zh-CN.md` §5 |
| BAR/CFG/ROM 访问 | `pcief_read()` / `pcief_cfg_read()` / `pcief_read_exp_rom()` | `test-suite-skill.zh-CN.md` §4.2 |
| IO 模式访问 | `pcief_io_read()` / `pcief_io_greg_u32()` | `test-suite-skill.zh-CN.md` §4.2 |
| DMA 缓冲区 | `pcief_dmabuf_malloc()` / `pcief_dmabuf_write()` | `test-suite-skill.zh-CN.md` §4.3 |
| 中断与 IPC | `pcief_irq_init()` / `pcief_wait_int()` / `pcief_tgt_cmd()` | `test-suite-skill.zh-CN.md` §4.4 |
| bare DMA | `pcief_dma_bare_xfer()` | `test-suite-skill.zh-CN.md` §4.5 / `driver-skill.zh-CN.md` §9 ~ §10 |
| DMA engine | `pcief_mtdma_engine_start()` | `test-suite-skill.zh-CN.md` §4.6 |
| target 命令 | `pcief_tgt_sreg_u32()` / `pcief_tgt_dma()` / `pcief_tgt_mtdma_reset()` | `test-suite-skill.zh-CN.md` §4.7 |
| 电源与模式 | `pcief_get_power()` / `pcief_suspend()` / `pcief_reg_mode()` | `test-suite-skill.zh-CN.md` §4.8 |
| 地址/性能 | `pcief_get_mtdma_info()` / `pcief_perf_slv_en()` | `test-suite-skill.zh-CN.md` §4.9 |

---

## 6. 数据结构与常量 -> skill 章节

| 结构/常量 | 源文件 | 优先查阅 |
|---|---|---|
| `struct emu_pcie` | `driver/mt-emu.h` | `driver-skill.zh-CN.md` §4 |
| `struct dma_bare_ch` / `struct dma_bare` | `driver/mt-emu.h` | `driver-skill.zh-CN.md` §4 |
| `struct dma_ch_desc` | `driver/mt-emu-mtdma-bare.h` | `driver-skill.zh-CN.md` §4 / §9 |
| `struct mt_emu_param` / `struct dma_bare_rw` / `struct mtdma_rw` | `driver/mt-emu-drv.h` | `driver-skill.zh-CN.md` §4 ~ §5 |
| `struct mtdma_engine_test_data` | `test/src/test_thr.h` | `test-suite-skill.zh-CN.md` §5.1 |
| `struct dma_bare_test_data` | `test/src/test_thr.h` | `test-suite-skill.zh-CN.md` §5.2 |
| `PCIEF_TGT_DSP/FEC/SMC` | `test/lib/mt_pcie_f.h` | `test-suite-skill.zh-CN.md` §4.7 |
| `PCIEF_D0` ~ `PCIEF_D3cold` | `test/lib/mt_pcie_f.h` | `test-suite-skill.zh-CN.md` §4.8 |

---

## 7. 已知兼容性问题 -> skill 章节

| 问题类型 | 优先查阅 |
|---|---|
| 新内核下 driver API 不兼容 | `environment-skill.zh-CN.md` §8.1 / `troubleshooting-skill.zh-CN.md` §3 |
| Catch2 / 编译器兼容性 | `environment-skill.zh-CN.md` §8.2 / `test-suite-skill.zh-CN.md` §12 / `troubleshooting-skill.zh-CN.md` §4 |
| 模块加载顺序/设备发现失败 | `environment-skill.zh-CN.md` §4 ~ §5 / `troubleshooting-skill.zh-CN.md` §5 |
| DMA / target 运行失败 | `driver-skill.zh-CN.md` §9 ~ §10 / `test-suite-skill.zh-CN.md` §4 ~ §7 / `troubleshooting-skill.zh-CN.md` §6 |
