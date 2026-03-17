# MT EMU PCIe DMA 驱动 — 编译构建与测试环境技能指南

> **用途**：本文件供 AI 工具（GitHub Copilot、Cursor 等）读取，
> 基于此可从零搭建完整的编译环境、加载驱动、并运行测试套件。
> 涵盖所有可变参数的传递方式，支持不同项目/设备的快速切换。
>
> ⚠️  本文件**不涵盖**驱动源码生成，请参阅 `skills/driver_skill.md`。
> ⚠️  本文件**不涵盖**测试用例生成，请参阅 `skills/test_skill.md`。
> ⚠️  忽略 `/examples/` 和 `/.github/` 目录下的所有内容。

---

## 1. 环境可变参数总表（切换项目 / 设备时在此处修改）

以下是所有需要人为按项目或设备调整的参数，AI 工具在生成脚本或配置时应优先引用这里的值。

```yaml
# ════════════════════════════════════════════════════════
#  A. 驱动编译参数（传给 driver/Makefile）
# ════════════════════════════════════════════════════════

# 【必须】虚函数（VF）数量
#  0  = 禁用 SRIOV，不生成 /dev/mt_emu_vgpu* 节点
#  最大值 = 60（由 VF_NUM_MAX 固定）
VF_NUM: 60

# 【必须】DDR 容量（GB）
#  默认 48，根据实际硬件填写
#  影响 DDR_SZ、DDR_SZ_FREE、LADDR_VGPU_BASE 等所有地址常量
DDR_SZ_GB: 48

# 【必须】DMA 保留内存模式
#  1 = 使用系统预留内存（MTDMA_BUF_SIZE = 32 GB，推荐生产环境）
#  0 = 动态分配（MTDMA_BUF_SIZE = 4 MB，适合仿真 / 小内存）
DMA_RESV_MEM: 1

# 【可选】扩展 ROM 支持（依赖 PHGOP.rom 文件）
#  1 = 启用   0 = 禁用（默认）
ROM_ENABLE: 0

# 【可选】调试标志（开发阶段建议全开）
ENABLE_DEBUG: true
ENABLE_VERBOSE_DEBUG: true
ENABLE_DEBUGFS: true

# ════════════════════════════════════════════════════════
#  B. PCIe 总线拓扑（用于 setpci / lspci / 设备枚举脚本）
# ════════════════════════════════════════════════════════

# 用 lspci 查找：lspci -D | grep 1ed5
PCIE_ROOT_BUS:    "17:00.0"       # Root Complex 端口地址
PCIE_EP_BUS_GPU:  "18:00.0"       # EP GPU PF0 地址（/dev/mt_emu_gpu）
PCIE_EP_BUS_APU:  "18:00.1"       # EP APU PF1 地址（/dev/mt_emu_apu）；无 APU 则注释

# ════════════════════════════════════════════════════════
#  C. 内核头文件路径（交叉编译或非标准内核时修改）
# ════════════════════════════════════════════════════════

# 默认 = 当前运行内核的头文件目录
KERNELDIR: "/lib/modules/$(uname -r)/build"

# ════════════════════════════════════════════════════════
#  D. 测试可执行文件路径（默认在 test/build/ 下）
# ════════════════════════════════════════════════════════
TEST_BIN: "./test/build/test"

# ════════════════════════════════════════════════════════
#  E. 日志与调试
# ════════════════════════════════════════════════════════

# dmesg 内核打印级别（8 = 显示所有级别）
KERNEL_PRINTK_LEVEL: 8

# hung_task 检测超时（0 = 禁用，避免长时间 DMA 被误报；非 0 值单位为秒）
HUNG_TASK_TIMEOUT: 0
```

---

## 2. 目录结构总览

```
pcie_driver/
├── driver/                  # Linux 内核模块源码（make 构建）
│   ├── Makefile             # 【关键】驱动构建入口
│   ├── mt-emu-drv.h         # 共享头文件（用户态+内核态共用）
│   └── *.c / *.h            # 驱动源文件（详见 driver_skill.md）
├── test/                    # 用户态测试套件（CMake 构建）
│   ├── CMakeLists.txt       # 【关键】测试构建入口
│   ├── src/                 # Catch2 测试用例（*.cc 自动收集）
│   └── lib/                 # 用户态 PCIe API 库
├── scripts/                 # 辅助脚本（安装 / 测试 / 调试）
│   ├── devmem.c             # 物理内存读写工具（独立编译）
│   ├── dump_desc.c          # DMA 描述符转储工具
│   ├── mtdma_reg_dump_pf0.c # PF0 DMA 寄存器转储
│   └── mtdma_reg_dump_vf.c  # VF DMA 寄存器转储
└── skills/                  # AI 技能文档（本目录）
```

---

## 3. 系统依赖安装

### 3.1 Ubuntu / Debian 系统

```bash
# 【必须】内核构建工具链
sudo apt update
sudo apt install -y \
    build-essential \
    linux-headers-$(uname -r) \
    linux-source \
    kmod

# 【必须】CMake + C++ 编译器（用于测试套件）
sudo apt install -y \
    cmake \
    g++ \
    gcc

# 【可选】PCIe 调试工具
sudo apt install -y \
    pciutils \        # lspci / setpci
    devmem2           # 物理内存读写
```

### 3.2 依赖版本要求

| 工具 | 最低版本 | 说明 |
|------|----------|------|
| GCC  | 7.0+     | 支持 C11/C++11 |
| CMake | 2.6+   | 测试套件构建 |
| Linux Kernel | 5.4+ | 支持 virt-dma 接口 |
| Python (可选) | 3.6+ | 若需调试脚本 |

---

## 4. 驱动模块构建

### 4.1 Makefile 参数说明

`driver/Makefile` 接受以下参数，对应第 1 节 A 组配置：

| 参数 | 默认值 | Makefile 变量 | 说明 |
|------|--------|--------------|------|
| VF_NUM | 60 | `VF_NUM ?= 60` | 虚函数数量，同时修改 `mt-emu-drv.h` |
| DDR_SZ_GB | 48 | 通过 `-DDDR_SZ_GB=N` 注入 | DDR 容量 |
| DMA_RESV_MEM | 1 | `-DDMA_RESV_MEM=N` | DMA 内存模式 |

> ⚠️  `VF_NUM` 的特殊处理：`make` 时会通过 `sed` 自动修改 `mt-emu-drv.h` 中的 `#define VF_NUM N`，
> 确保内核模块与头文件中的值一致。

### 4.2 标准构建命令

```bash
# 进入驱动目录
cd /path/to/pcie_driver/driver

# 使用第 1 节 A 组参数构建（替换 {参数} 为实际值）
make VF_NUM={VF_NUM} DMA_RESV_MEM={DMA_RESV_MEM}

# 指定非标准内核头文件路径
make KERNELDIR={KERNELDIR} VF_NUM={VF_NUM}

# 清理构建产物
make clean
```

### 4.3 构建产物

构建成功后在 `driver/` 目录下生成四个 `.ko` 文件：

```
driver/
├── mt_emu_mtdma.ko   # DMA Engine 核心层（必须最先加载）
├── mt_emu_gpu.ko     # PF0 GPU 功能驱动
├── mt_emu_apu.ko     # PF1 APU 功能驱动
└── mt_emu_vgpu.ko    # VF 虚拟功能驱动（VF_NUM 个）
```

---

## 5. 测试套件构建

### 5.1 CMakeLists.txt 关键逻辑

```cmake
# 头文件搜索路径（包含驱动头文件）
include_directories(../driver lib util ../driver/pcie ...)

# 自动收集所有测试源文件（新建 .cc 文件无需手动添加）
file(GLOB TEST_SOURCES src/*.cc)
add_executable(test ${TEST_SOURCES})

# 静态库（PCIe 用户态 API）
file(GLOB LIB_SOURCES lib/*.cc lib/*.c util/*.c)
add_library(mlib STATIC ${LIB_SOURCES})

# 链接库
target_link_libraries(test mlib ${CMAKE_THREAD_LIBS_INIT})
```

### 5.2 标准构建命令

```bash
# 进入测试目录，创建构建目录
cd /path/to/pcie_driver/test
mkdir -p build && cd build

# Debug 构建（开发阶段，保留调试信息）
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Release 构建（性能测试 / 生产环境）
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 5.3 构建产物

```
test/build/
├── test       # 主测试可执行文件（Catch2 框架）
├── shell      # 交互式 shell 程序
└── libmlib.a  # 静态 PCIe API 库
```

---

## 6. 驱动模块加载（insmod 顺序固定）

### 6.1 加载顺序要求

```
第 1 步：mt_emu_mtdma.ko   ← DMA Engine 核心（其余模块依赖此符号）
第 2 步：mt_emu_gpu.ko     ← PF0（会探测硬件，注册设备节点）
第 3 步：mt_emu_apu.ko     ← PF1（需 GPU 驱动先注册 DMA 控制器）
第 4 步：mt_emu_vgpu.ko    ← VF（需 GPU 驱动已设置 SRIOV）
```

> ⚠️  **颠倒顺序会导致 insmod 报错或设备初始化失败。**

### 6.2 加载命令

```bash
# 系统调优（避免误报）
echo {KERNEL_PRINTK_LEVEL} > /proc/sys/kernel/printk
echo {HUNG_TASK_TIMEOUT} > /proc/sys/kernel/hung_task_timeout_secs

# 卸载旧模块（忽略未加载时的报错）
sudo rmmod mt_emu_vgpu  2>/dev/null || true
sudo rmmod mt_emu_apu   2>/dev/null || true
sudo rmmod mt_emu_gpu   2>/dev/null || true
sudo rmmod mt_emu_mtdma 2>/dev/null || true

# 按顺序加载
DRV_DIR=/path/to/pcie_driver/driver
sudo insmod ${DRV_DIR}/mt_emu_mtdma.ko
sudo insmod ${DRV_DIR}/mt_emu_gpu.ko
sudo insmod ${DRV_DIR}/mt_emu_apu.ko
sudo insmod ${DRV_DIR}/mt_emu_vgpu.ko

# 验证设备节点
ls -la /dev/mt_emu*
```

### 6.3 预期设备节点

加载成功后，应出现以下设备节点：

| 设备节点 | 来源模块 | 用途 |
|---------|---------|------|
| `/dev/mt_emu_gpu` | mt_emu_gpu.ko | PF0 主设备（裸机 DMA 入口） |
| `/dev/mt_emu_apu` | mt_emu_apu.ko | PF1 APU 设备 |
| `/dev/mt_emu_vgpu0` ~ `/dev/mt_emu_vgpu{VF_NUM-1}` | mt_emu_vgpu.ko | VF 虚拟设备 |
| `/dev/mt_emu_dmabuf` | mt_emu_gpu.ko | 主机侧 DMA 缓冲区管理 |

---

## 7. 完整环境搭建流程（一键脚本模板）

AI 工具生成 `scripts/setup_env.sh` 时，参照以下模板，并将 `{参数}` 替换为第 1 节的实际值：

```bash
#!/bin/bash
# ======================================================
# MT EMU PCIe DMA 环境搭建脚本
# 【可变参数】对应 skills/env_skill.md 第 1 节
# ======================================================
set -e

# ── 可变参数（切换项目时修改此处）──────────────────────
VF_NUM={VF_NUM}                         # 虚函数数量
DDR_SZ_GB={DDR_SZ_GB}                   # DDR 容量 GB
DMA_RESV_MEM={DMA_RESV_MEM}             # 1=保留内存模式
KERNEL_PRINTK_LEVEL={KERNEL_PRINTK_LEVEL}  # 内核日志级别
HUNG_TASK_TIMEOUT={HUNG_TASK_TIMEOUT}   # hung_task 超时（0=禁用）
# ──────────────────────────────────────────────────────

MYDIR="$(cd "$(dirname "$0")/.." && pwd)"
DRV_DIR="${MYDIR}/driver"
TEST_DIR="${MYDIR}/test"
LOG_FILE="${MYDIR}/setup_env.log"

echo "=== MT EMU PCIe 环境搭建 ===" | tee "${LOG_FILE}"
echo "参数：VF_NUM=${VF_NUM} DDR_SZ_GB=${DDR_SZ_GB} DMA_RESV_MEM=${DMA_RESV_MEM}" | tee -a "${LOG_FILE}"

# ── 步骤 1：安装系统依赖 ─────────────────────────────
echo "[1/4] 安装系统依赖..." | tee -a "${LOG_FILE}"
sudo apt-get update -qq
sudo apt-get install -y build-essential cmake g++ gcc \
    linux-headers-$(uname -r) kmod pciutils 2>&1 | tee -a "${LOG_FILE}"

# ── 步骤 2：编译驱动模块 ─────────────────────────────
echo "[2/4] 编译驱动模块（VF_NUM=${VF_NUM}）..." | tee -a "${LOG_FILE}"
cd "${DRV_DIR}"
make clean
make VF_NUM=${VF_NUM} DMA_RESV_MEM=${DMA_RESV_MEM} 2>&1 | tee -a "${LOG_FILE}"

# ── 步骤 3：编译测试套件 ─────────────────────────────
echo "[3/4] 编译测试套件..." | tee -a "${LOG_FILE}"
rm -rf "${TEST_DIR}/build"
mkdir -p "${TEST_DIR}/build"
cd "${TEST_DIR}/build"
cmake -DCMAKE_BUILD_TYPE=Debug .. 2>&1 | tee -a "${LOG_FILE}"
make -j$(nproc) 2>&1 | tee -a "${LOG_FILE}"

# ── 步骤 4：加载驱动模块 ─────────────────────────────
echo "[4/4] 加载驱动模块..." | tee -a "${LOG_FILE}"
echo ${KERNEL_PRINTK_LEVEL} > /proc/sys/kernel/printk  || true
echo ${HUNG_TASK_TIMEOUT}   > /proc/sys/kernel/hung_task_timeout_secs || true

# 启用动态调试（可按需注释）
echo 'module mt_emu_gpu +p' > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true

# 卸载旧模块
sudo rmmod mt_emu_vgpu  2>/dev/null || true
sudo rmmod mt_emu_apu   2>/dev/null || true
sudo rmmod mt_emu_gpu   2>/dev/null || true
sudo rmmod mt_emu_mtdma 2>/dev/null || true
sudo dmesg -c > /dev/null

# 按顺序加载
sudo insmod "${DRV_DIR}/mt_emu_mtdma.ko"
sudo insmod "${DRV_DIR}/mt_emu_gpu.ko"
sudo insmod "${DRV_DIR}/mt_emu_apu.ko"
sudo insmod "${DRV_DIR}/mt_emu_vgpu.ko"

# ── 验证 ─────────────────────────────────────────────
echo "=== 设备节点 ===" | tee -a "${LOG_FILE}"
ls -la /dev/mt_emu* 2>&1 | tee -a "${LOG_FILE}"

echo "=== dmesg 末尾 30 行 ===" | tee -a "${LOG_FILE}"
dmesg | tail -30 | tee -a "${LOG_FILE}"

cd "${TEST_DIR}/build"
echo ""
echo "✅ 环境搭建完成！测试二进制：${TEST_DIR}/build/test"
echo "   运行示例：./test -t \"[mtdma0]\""
```

---

## 8. 驱动卸载脚本模板

```bash
#!/bin/bash
# ======================================================
# MT EMU PCIe 驱动卸载脚本
# ======================================================
set -e

MYDIR="$(cd "$(dirname "$0")/.." && pwd)"
DRV_DIR="${MYDIR}/driver"

echo "=== 卸载驱动模块 ==="

# 卸载顺序与加载顺序相反
sudo rmmod mt_emu_vgpu  2>/dev/null || echo "  mt_emu_vgpu 未加载，跳过"
sudo rmmod mt_emu_apu   2>/dev/null || echo "  mt_emu_apu 未加载，跳过"
sudo rmmod mt_emu_gpu   2>/dev/null || echo "  mt_emu_gpu 未加载，跳过"
sudo rmmod mt_emu_mtdma 2>/dev/null || echo "  mt_emu_mtdma 未加载，跳过"

echo "=== 清理构建产物 ==="
cd "${DRV_DIR}" && make clean
rm -rf "${MYDIR}/test/build"

echo "✅ 卸载完成"
```

---

## 9. 测试运行命令参考

测试二进制使用 **Catch2** 框架，支持按标签、名称或全量运行。

### 9.1 按标签运行

```bash
# 进入测试构建目录
cd /path/to/pcie_driver/test/build

# 冒烟测试（最快，每次改动后必跑）
./test -t "[mtdma0]"

# DMA 功能测试（链式、块模式）
./test -t "[mtdma1]"

# MMU + DMA 功能测试
./test -t "[mtdma_mmu]"

# BAR/ROM/SRAM 基础读写测试
./test -t "[base]"
./test -t "[ph1s_base]"
./test -t "[ph_base]"

# 中断测试
./test -t "[intr0]"      # PF0 软中断基础
./test -t "[intr1]"      # 中断路由映射

# 压力测试
./test -t "[ph1s_stress]"   # SRAM 随机压力
./test -t "[ph_stress]"     # BAR0 SRAM 压力
./test -t "[stress]"        # 24 小时 DMA 压力

# 性能测试
./test -t "[perf]"

# 健全性测试（多 PCIe 速度等级）
./test -t "[sanity]"

# 列出所有可用标签和测试名称
./test --list-tags
./test --list-tests
```

### 9.2 运行单个测试用例

```bash
# 按测试用例名称运行
./test "sanity_dma_bare_single_s"

# 模糊匹配（包含关键词的所有用例）
./test -t "[mtdma*]"
```

### 9.3 并发压力测试

```bash
# 启动 3 个并行实例，写入独立日志（用于 24 小时压力测试）
./test -t "[stress]" > stress0.log 2>&1 &
./test -t "[stress]" > stress1.log 2>&1 &
./test -t "[stress]" > stress2.log 2>&1 &
wait
echo "压力测试完成"
```

---

## 10. PCIe 速度切换脚本模板（健全性测试）

用于在 Gen1 / Gen2 / Gen3 / Gen4 之间切换并验证，依赖第 1 节 B 组的总线地址：

```bash
#!/bin/bash
# ======================================================
# PCIe 速度切换 + 健全性测试脚本
# 【可变参数】对应 skills/env_skill.md 第 1 节 B 组
# ======================================================

# ── 可变参数（切换项目时修改此处）──
RC="{PCIE_ROOT_BUS}"                # 例：17:00.0
EP_GPU="{PCIE_EP_BUS_GPU}"          # 例：18:00.0
EP_APU="{PCIE_EP_BUS_APU}"          # 例：18:00.1
TEST_BIN="{TEST_BIN}"               # 例：/path/to/test/build/test
# ──────────────────────────────────────────────────────

run_sanity_at_gen() {
    local gen=$1
    local log="sanity_gen${gen}.log"

    echo "=== PCIe Gen${gen} 健全测试 ===" | tee "${log}"

    # 切换 EP 速度等级
    setpci -s "${EP_GPU}" cap10+30.b="${gen}"
    # 触发 RC 端口链路重新训练
    setpci -s "${RC}" 3e.b=42
    setpci -s "${RC}" 3e.b=2
    sleep 10

    # 确认当前链路速度
    lspci -vvvs "${EP_GPU}" | grep -i lnk | tee -a "${log}"

    # 移除设备节点并重新扫描（相当于热插拔）
    local ep_gpu_pci="0000:${EP_GPU//:\//.}"
    local ep_apu_pci="0000:${EP_APU//:\//.}"
    local rc_pci="0000:${RC//:\//.}"
    echo 1 > /sys/bus/pci/devices/${ep_gpu_pci}/remove 2>/dev/null || true
    echo 1 > /sys/bus/pci/devices/${ep_apu_pci}/remove 2>/dev/null || true
    echo 1 > /sys/bus/pci/devices/${rc_pci}/rescan     2>/dev/null || true
    sleep 2

    # 重新加载驱动
    source "$(dirname "$0")/install.sh" 2>/dev/null || true

    # 运行健全测试
    "${TEST_BIN}" -t "[sanity]" 2>&1 | tee -a "${log}"
}

for gen in 4 3 2 1; do
    run_sanity_at_gen "${gen}"
done

echo "✅ 所有速度等级健全测试完成"
```

---

## 11. 调试辅助工具

### 11.1 `scripts/devmem.c` — 物理内存读写工具

```bash
# 编译
gcc -o devmem scripts/devmem.c

# 读取物理地址 0x800000 处的 32 位值
sudo ./devmem 0x800000

# 写入物理地址 0x800010 处的值 0x12345678
sudo ./devmem 0x800010 32 0x12345678
```

### 11.2 `scripts/mtdma_reg_dump_pf0.c` — PF0 DMA 寄存器转储

```bash
# 编译（注意：需要 DMA_REG 基地址宏，来自 driver/mt-emu-mtdma-bare.h）
gcc -I../driver -o mtdma_reg_dump_pf0 scripts/mtdma_reg_dump_pf0.c

# 运行：转储所有 PF0 DMA 通道寄存器到标准输出
sudo ./mtdma_reg_dump_pf0
```

### 11.3 `scripts/dump_desc.c` — DMA 描述符转储

```bash
# 编译
gcc -o dump_desc scripts/dump_desc.c

# 运行：从设备内存转储链表描述符
sudo ./dump_desc
```

### 11.4 dmesg 过滤

```bash
# 过滤 DMA 相关日志
dmesg | grep -E "mtdma|mt_emu|DMA int|bare"

# 实时监控
dmesg -w | grep -E "mtdma|mt_emu"

# 清空并重新捕获（加载驱动前先清空）
sudo dmesg -c
```

---

## 12. 常见问题排查

| 现象 | 可能原因 | 排查命令 |
|------|----------|---------|
| `insmod: ERROR: could not insert module` | 加载顺序错误，或依赖符号未找到 | `dmesg \| tail -20` |
| `/dev/mt_emu_gpu` 不存在 | mt_emu_gpu.ko 未正确加载或硬件未连接 | `lspci \| grep 1ed5` |
| ioctl 返回 -ENOENT | 设备节点路径错误 | `ls -la /dev/mt_emu*` |
| DMA 超时 | MSI-X 中断未到达 / IRQ 未初始化 | `dmesg \| grep -i irq` |
| DMA 数据错误 | SAR/DAR 地址越界 | 检查 DDR_SZ_FREE 与实际 DDR 大小 |
| VF 设备节点缺失 | VF_NUM=0 或 SRIOV 未启用 | `lspci -vvv \| grep -i sriov` |
| 内核编译错误 `unknown symbol` | KERNELDIR 指向错误的内核头文件 | `uname -r` vs `ls /lib/modules/` |
| CMA 分配失败 | DMA_RESV_MEM=0 且内核 CMA 区域不足 | 改为 `DMA_RESV_MEM=1` |

---

## 13. 参数传递路径总结

下图展示了第 1 节各组参数如何流入不同组件：

```
skills/env_skill.md 第 1 节
│
├── A 组（驱动编译参数）
│   └── make VF_NUM=N DMA_RESV_MEM=N
│         └── driver/Makefile
│               ├── sed 修改 mt-emu-drv.h 中的 #define VF_NUM
│               └── ccflags-y += -DDDR_SZ_GB=N -DDMA_RESV_MEM=N
│                     └── 编译进内核模块 *.ko
│
├── B 组（PCIe 总线拓扑）
│   └── scripts/test_sanity.sh / setup_env.sh
│         └── setpci / lspci 命令参数
│
├── C 组（内核头文件路径）
│   └── make KERNELDIR=/path/to/headers
│
└── D 组（测试二进制路径）
      └── scripts/*.sh 中的 TEST_BIN 变量
```

---

## 14. 切换到新项目/设备的最小改动清单

```
□ 1. 修改 env_skill.md 第 1 节参数
      - VF_NUM     → 目标设备的 VF 数量
      - DDR_SZ_GB  → 目标设备的 DDR 容量
      - PCIE_EP_BUS_GPU / PCIE_EP_BUS_APU → 新的 lspci 地址
□ 2. 确认 driver_skill.md 第 1 节中 ACTIVE_GPU_DEVICE_ID 与实际硬件匹配
□ 3. 重新运行 scripts/setup_env.sh（自动完成编译 + 加载）
□ 4. 运行冒烟测试：./test/build/test -t "[mtdma0]"
□ 5. 若 DMA 测试失败，检查 dmesg 中的 DMA 中断日志
```
