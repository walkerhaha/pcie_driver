---
title: "Skill：PCIe & DMA 构建与测试环境重建"
version: "1.1"
last_modified: "2026-03-17"
dependencies: []
target_audience: "AI Agent / Project Replicator"
maintenance_checklist_complete: false
status: "usable"
source_scope:
  - "README.md"
  - "driver/Makefile"
  - "test/CMakeLists.txt"
  - "scripts/"
  - "test/lib/mt_pcie_f.c"
excludes:
  - "examples/"
---

# Skill：PCIe & DMA 构建与测试环境重建

## 1. 目标

定义一个**可参数化的环境 skill**，使其它项目不继承本仓库脚本实现，也能从零搭建：

- 驱动编译环境
- 用户态测试编译环境
- 驱动加载与设备节点发现环境
- 冒烟/回归/压力测试运行环境

本 skill 的依据仅来自：

- `README.md`
- `driver/Makefile`
- `test/CMakeLists.txt`
- `scripts/bulid.sh`（仓库当前文件名如此，后续新项目可规范命名为 `build.sh`）
- `scripts/install.sh`
- `scripts/test_sanity.sh`
- `test/lib/mt_pcie_f.c`

说明：本文正文统一使用“build”描述构建动作；只有在引用仓库现有脚本文件时，才按真实文件名写作 `scripts/bulid.sh`。

---

## 2. 环境分层

### 2.1 构建依赖

来源：`scripts/bulid.sh`（现名）

- `build-essential`
- `cmake`
- `linux-source`
- `linux-headers-$(uname -r)`

### 2.2 驱动编译入口

来源：`README.md`、`driver/Makefile`

```bash
cd driver
make
```

### 2.3 测试编译入口

来源：`README.md`、`test/CMakeLists.txt`

```bash
cd test
mkdir -p build && cd build
cmake ..
make
```

### 2.4 驱动加载入口

来源：`scripts/install.sh`

固定加载顺序：

```bash
sudo insmod driver/mt_emu_mtdma.ko
sudo insmod driver/mt_emu_gpu.ko
sudo insmod driver/mt_emu_apu.ko
sudo insmod driver/mt_emu_vgpu.ko
```

---

## 3. 必须外部传入的环境参数

未来项目不能把这些参数写死在代码里，应作为环境 skill 的输入。

### 3.1 驱动编译参数

来源：`driver/Makefile`

| 参数 | 作用 | 当前默认值 |
|---|---|---|
| `KERNELDIR` | 内核头/内核构建目录 | `/lib/modules/$(uname -r)/build` |
| `VF_NUM` | VF 数量 | `60` |
| `DMA_RESV_MEM` | 是否使用保留物理内存作为 DMA 大缓冲 | `1` |
| `DDR_SZ_GB` | DDR 逻辑容量 | `48` |
| `ROM_ENABLE` | 是否映射扩展 ROM | 脚本里显式传 `0` |

推荐以后统一用以下配置对象传入：

```yaml
driver_build:
  kerneldir: /lib/modules/<kernel>/build
  vf_num: 0
  dma_resv_mem: 1
  ddr_sz_gb: 32
  rom_enable: 0
```

### 3.2 测试编译参数

来源：`test/CMakeLists.txt`

| 参数 | 作用 | 当前值 |
|---|---|---|
| `CMAKE_BUILD_TYPE` | Debug/Release | 默认走 Debug 路径 |
| `CMAKE_CXX_COMPILER` | C++ 编译器 | `g++` |
| `CMAKE_C_COMPILER` | C 编译器 | `gcc` |

推荐配置对象：

```yaml
test_build:
  build_type: Debug
  c_compiler: gcc
  cxx_compiler: g++
```

### 3.3 运行时硬件参数

来源：`scripts/test_sanity.sh`、`test/lib/mt_pcie_f.c`

| 参数 | 作用 |
|---|---|
| `gpu_bdf` | PF0 BDF，如 `18:00.0` |
| `apu_bdf` | PF1 BDF，如 `18:00.1` |
| `root_port_bdf` | Root port BDF，如 `17:00.0` |
| `pcie_gen` | 目标链路速率，1~5 |
| `vf_num` | 实际启用 VF 数量 |
| `reserved_mem_start` | 预留 DMA 物理起始地址，当前代码中为 `0x400000000` |
| `reserved_mem_size` | DMA 预留大小，对应 `MTDMA_BUF_SIZE` |

推荐配置对象：

```yaml
runtime_hw:
  gpu_bdf: "18:00.0"
  apu_bdf: "18:00.1"
  root_port_bdf: "17:00.0"
  pcie_gen: 4
  vf_num: 0
  reserved_mem_start: 0x400000000
  reserved_mem_size: 0x800000000
```

---

## 4. 环境准备流程

## 4.1 系统包

```bash
sudo apt install -y build-essential cmake linux-source linux-headers-$(uname -r)
```

## 4.2 编译驱动

```bash
cd <repo>/driver
make clean
make VF_NUM=<vf_num> DMA_RESV_MEM=<dma_resv_mem> DDR_SZ_GB=<ddr_sz_gb> ROM_ENABLE=<rom_enable>
```

## 4.3 编译测试

```bash
cd <repo>/test
rm -rf build
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=<build_type> ..
make
```

## 4.4 加载驱动

```bash
sudo rmmod mt_emu_vgpu || true
sudo rmmod mt_emu_apu || true
sudo rmmod mt_emu_gpu || true
sudo rmmod mt_emu_mtdma || true

sudo insmod driver/mt_emu_mtdma.ko
sudo insmod driver/mt_emu_gpu.ko
sudo insmod driver/mt_emu_apu.ko
sudo insmod driver/mt_emu_vgpu.ko
```

## 4.5 运行前检查

必须确认：

- `/dev/mt_emu_gpu`
- `/dev/mt_emu_dmabuf`
- `/sys/class/misc/mt_emu_gpu/bar0`
- `/sys/class/misc/mt_emu_gpu/bar2`

如果启用了 VF，还要确认：

- `/dev/mt_emu_vgpu<N>`

---

## 5. 用户态运行环境的隐含约定

来源：`test/lib/mt_pcie_f.c`

### 5.1 设备发现方式

用户态并不是硬编码 BAR，而是通过以下路径发现：

- `/dev/mt_emu_gpu`
- `/dev/mt_emu_dmabuf`
- `/dev/mt_emu_vgpu<N>`
- `/sys/class/misc/<device>/bar0`
- `/sys/class/misc/<device>/bar2`
- `/sys/class/misc/mt_emu_gpu/vf`

### 5.2 `pcief_init()` 的环境要求

`pcief_init()` 会：

1. 初始化全局互斥锁
2. 逐个尝试打开所有 function 设备
3. 读取 sysfs 中的 `bar*` 元数据
4. `mmap` BAR 空间
5. 读取 VF 数量

所以未来项目如果重建测试环境，必须保留：

- misc 设备
- bar 元数据 sysfs
- 支持 `mmap`

---

## 6. 建议保留的脚本级动作

### 6.1 动态调试

来源：`scripts/install.sh`

```bash
echo 'module mt_emu_gpu +p' > /sys/kernel/debug/dynamic_debug/control
```

### 6.2 调整 printk 与 hung task

来源：`scripts/install.sh`

```bash
echo 8  > /proc/sys/kernel/printk
echo 0 > /proc/sys/kernel/hung_task_timeout_secs
```

### 6.3 切换链路速率并重扫

来源：`scripts/test_sanity.sh`

```bash
setpci -s <gpu_bdf> cap10+30.b=<pcie_gen>
setpci -s <root_port_bdf> 3e.b=42
setpci -s <root_port_bdf> 3e.b=2
sleep 10s
echo 1 > /sys/bus/pci/devices/0000:<gpu_bdf>/remove
echo 1 > /sys/bus/pci/devices/0000:<apu_bdf>/remove
echo 1 > /sys/bus/pci/devices/0000:<root_port_bdf>/rescan
```

---

## 7. 测试运行参数契约

未来项目建议把所有测试输入统一成一个对象，而不是散落在脚本、源码和命令行里。

```yaml
test_runtime:
  catch_filter: "[mtdma0]"
  irq_mode: "msi"   # legacy/msi/msix
  irq_test_mode: 1
  timeout_ms: 30000
  pcie_gen: 4
  need_rescan: true
```

---

## 8. 当前仓库在本地环境下的基线构建状态

为便于未来迁移，需记录当前仓库与新环境的已知耦合点：

### 8.1 driver 基线问题

在当前内核头 `6.14.0-1017-azure` 下，`driver/make` 失败，主要因为旧 API 与新内核不兼容，例如：

- `iommu_map` 参数个数不匹配
- `pci_enable_pcie_error_reporting` 在当前头环境下未声明

### 8.2 test 基线问题

`test/build && cmake .. && make` 在当前工具链下失败，命中：

- `test/src/catch2/catch.hpp` 的 `sysconf()` constexpr 相关错误
- `altStackMem` 不是 integral constant expression

结论：**环境 skill 应把“工具链/内核版本兼容性”作为外部变量管理，而不是假定仓库能在任意新环境直接通过。**

---

## 9. 推荐的环境重建顺序

1. 先定义输入参数对象
2. 再安装系统依赖
3. 再编译 driver/test
4. 再加载模块并校验 `/dev` 与 `/sys/class/misc`
5. 最后运行 Catch2 标签测试

---

## 10. 人工更新检查表

- [ ] `driver/Makefile` 可调参数是否变化
- [ ] `test/CMakeLists.txt` 编译器/标准是否变化
- [ ] 模块加载顺序是否变化
- [ ] 设备节点命名是否变化
- [ ] sysfs `bar*` / `vf` 属性是否变化
- [ ] 测试脚本中的 BDF / 链路切速流程是否变化
- [ ] 新内核/新编译器兼容性问题是否需要在 skill 中补充
