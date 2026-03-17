# MT EMU PCIe DMA 驱动 — 驱动文件与脚本生成技能指南

> **用途**：本文件供 AI 工具（GitHub Copilot、Cursor 等）读取，
> 基于此可生成完整的内核驱动文件（`driver/`）和配套脚本（`scripts/`）。
> 切换项目或设备时，只需修改第 1 节的"项目配置项"，
> AI 工具即可据此重新生成适配代码。
>
> ⚠️  本文件**不涵盖**测试集生成，测试集请参阅 `skills/test_skill.md`。
> ⚠️  忽略 `/examples/` 和 `/.github/` 目录下的所有内容。

---

## 1. 项目配置项（切换项目 / 设备时在此处修改）

以下是所有需要人为按项目或设备调整的参数，AI 工具在生成代码时应优先引用这里的值。

```yaml
# ────────────────────────────────────────────────────────
#  【必须】PCI 设备标识
# ────────────────────────────────────────────────────────
VENDOR_ID:        0x1ed5          # 摩尔线程厂商 ID（固定不变）

# 目标设备 ID 列表（保留全部，注释掉不用的）
# QY（曲渊）:    GPU=0x0200, APU=0x0201
# QY2（曲渊2）:  GPU=0x0400
# PH1S:           GPU=0x0500
# HS（高速）:    GPU=0x0680
# LS（低速）:    GPU=0x0610
ACTIVE_GPU_DEVICE_ID: 0x0500      # 当前激活的 GPU 设备 ID
ACTIVE_APU_DEVICE_ID: 0x0501      # 当前激活的 APU 设备 ID（若无 APU 则注释）

# ────────────────────────────────────────────────────────
#  【必须】虚函数（VF）数量
#  0  = 禁用 SRIOV / 不启用虚函数
#  最大值 = 60
# ────────────────────────────────────────────────────────
VF_NUM: 0

# ────────────────────────────────────────────────────────
#  【必须】DDR 容量（GB）
#  默认 48，可按实际硬件设置
# ────────────────────────────────────────────────────────
DDR_SZ_GB: 48

# ────────────────────────────────────────────────────────
#  【必须】DMA 保留内存模式
#  1 = 使用 CMA/预留内存（推荐，无需内核 CMA 补丁）
#  0 = 动态分配（需要内核 CMA 支持）
# ────────────────────────────────────────────────────────
DMA_RESV_MEM: 1

# ────────────────────────────────────────────────────────
#  【可选】ROM 支持
#  1 = 启用扩展 ROM 读取（需要 PHGOP.rom 文件）
#  0 = 禁用
# ────────────────────────────────────────────────────────
ROM_ENABLE: 0

# ────────────────────────────────────────────────────────
#  【可选】PCIe 总线拓扑（用于脚本中的 setpci / lspci）
#  根据实际系统 lspci 输出填写
# ────────────────────────────────────────────────────────
PCIE_ROOT_BUS:    "17:00.0"       # RC（Root Complex）端口
PCIE_EP_BUS_GPU:  "18:00.0"       # EP GPU 设备地址
PCIE_EP_BUS_APU:  "18:00.1"       # EP APU 设备地址（若无则注释）

# ────────────────────────────────────────────────────────
#  【可选】调试开关
#  编译时是否开启 DEBUG / VERBOSE_DEBUG
# ────────────────────────────────────────────────────────
ENABLE_DEBUG:        true
ENABLE_VERBOSE_DEBUG: true
ENABLE_DEBUGFS:      true
```

---

## 2. 驱动模块总体架构

本项目编译生成四个内核模块（`.ko`），**加载顺序不可颠倒**：

```
1. mt_emu_mtdma.ko   ← DMA Engine 核心层（virt-dma 封装）
2. mt_emu_gpu.ko     ← PF0 GPU 功能驱动（BAR 映射、ioctl、裸机 DMA、中断）
3. mt_emu_apu.ko     ← PF1 APU 功能驱动
4. mt_emu_vgpu.ko    ← 虚函数（VF 0~VF_NUM-1）驱动
```

### 模块依赖关系图

```
mt_emu_mtdma.ko
    └── mt-emu-mtdma-core.c   (DMA Engine 通道管理)
    └── virt-dma.c            (Linux virt-dma 抽象)

mt_emu_gpu.ko
    ├── mt-emu-gpu.c          (PCI probe/remove, BAR 映射)
    ├── mt-emu-ioctl.c        (ioctl 命令分发)
    ├── mt-emu-dmabuf.c       (主机侧 DMA 缓冲区)
    ├── mt-emu-intr.c         (MSI-X 中断)
    ├── mt-emu-mtdma-bare.c   (裸机 DMA 传输)
    ├── mt-emu-mtdma-test.c   (DMA Engine 测试封装)
    ├── eata_api.c            (EATA 协议)
    └── mmu_init_pagetable.c  (MMU 页表初始化)

mt_emu_apu.ko
    ├── mt-emu-apu.c
    ├── mt-emu-ioctl.c
    ├── mt-emu-mtdma-bare.c
    └── mt-emu-mtdma-test.c

mt_emu_vgpu.ko
    ├── mt-emu-vgpu.c
    ├── mt-emu-ioctl.c
    ├── mt-emu-mtdma-bare.c
    └── mt-emu-mtdma-test.c
```

---

## 3. 关键头文件与数据结构

### 3.1 `driver/mt-emu-drv.h` — 主驱动头文件

此文件包含用户态/内核态共用的常量、枚举、ioctl 结构体。

**PCI 设备 ID 宏（根据配置项 `ACTIVE_GPU_DEVICE_ID` 选择）：**

```c
#define PCI_VENDOR_ID_MT            0x1ed5

/* 曲渊（QY）*/
#define PCI_DEVICE_ID_MT_QY_GPU     0x0200
#define PCI_DEVICE_ID_MT_QY_APU     0x0201

/* 曲渊2（QY2）*/
#define PCI_DEVICE_ID_MT_QY2_GPU    0x0400

/* PH1S */
#define PCI_DEVICE_ID_MT_PH1S_GPU   0x0500

/* 高速（HS）*/
#define PCI_DEVICE_ID_MT_HS_GPU     0x0680

/* 低速（LS）*/
#define PCI_DEVICE_ID_MT_LS_GPU     0x0610
```

**关键编译期常量（由 Makefile 参数注入）：**

```c
#define VF_NUM          60          /* 可通过 make VF_NUM=N 覆盖 */
#define DDR_SZ_GB       48          /* 可通过 make DDR_SZ_GB=N 覆盖 */
#define PCIE_DMA_CH_NUM 64          /* 每个 PF 的 DMA 通道数，固定 */

/* DDR 布局 */
#define DDR_SZ          (0x40000000ULL * DDR_SZ_GB)
#define DDR_SZ_RESV     0x0040000000ULL   /* 1 GB 保留 */
#define DDR_SZ_FREE     (DDR_SZ - DDR_SZ_RESV)

/* 关键地址常量 */
#define LADDR_APU           (DDR_SZ_FREE)
#define LADDR_MTDMA_LL_WR   (DDR_SZ_FREE + 0x2000000)
#define LADDR_MTDMA_LL_RD   (LADDR_MTDMA_LL_WR + 0x10000)
#define LADDR_MTDMA_TEST    0x100000       /* 标准测试区基地址 */

/* VF DDR 窗口 */
#define SIZE_VGPU_DDR   0x40000000ULL      /* 每个 VF 1 GB */
#define LADDR_VGPU(vf)  (LADDR_VGPU_BASE + SIZE_VGPU_DDR * (vf))
```

**ioctl 命令（与用户态测试库对应）：**

```c
#define MT_IOCTL_BAR_RW         /* BAR 读写 */
#define MT_IOCTL_CFG_RW         /* 配置空间读写 */
#define MT_IOCTL_IRQ_INIT       /* 中断初始化 */
#define MT_IOCTL_WAIT_INT       /* 等待中断 */
#define MT_IOCTL_DMAISR_SET     /* DMA ISR 配置 */
#define MT_IOCTL_MTDMA_BARE_RW  /* 裸机 DMA 传输（核心命令） */
#define MT_IOCTL_MTDMA_RW       /* DMA Engine 模式传输 */
```

**核心 ioctl 数据结构：**

```c
/* 裸机 DMA 传输参数 */
struct dma_bare_rw {
    uint64_t sar;              /* 源地址 */
    uint64_t dar;              /* 目的地址 */
    uint32_t data_direction;   /* 方向：0=H2H, 1=H2D, 2=D2H, 3=D2D */
    uint32_t desc_direction;   /* 描述符位置：0=设备, 1=主机 */
    uint32_t desc_cnt;         /* N 个描述符时填 N-1；单描述符填 0 */
    uint32_t block_cnt;        /* 块数量；0=不使用块模式 */
    uint32_t size;             /* 传输总字节数 */
    uint32_t ch_num;           /* 通道号 0-63 */
    uint32_t timeout_ms;       /* 超时毫秒数 */
};

/* DMA Engine 模式传输参数 */
struct mtdma_rw {
    unsigned long long laddr;  /* 设备本地地址（DDR） */
    unsigned long long size;   /* 传输字节数 */
    unsigned int timeout_ms;
    unsigned int test_cnt;
    unsigned int ch;           /* 通道号 */
    unsigned int dir;          /* 0=写(H2D), 1=读(D2H) */
};
```

### 3.2 `driver/mt-emu-mtdma-bare.h` — 裸机 DMA 头文件

**DMA 控制器寄存器基地址：**

```c
#define REG_DMA_CHAN_BASE  0x383000   /* 通道寄存器基地址 */
#define REG_DMA_COMM_BASE  0x380000   /* 公共寄存器基地址 */
```

**通道寄存器偏移（相对每通道基地址，步长 0x1000）：**

```c
#define REG_DMA_CH_ENABLE       0x000   /* bit0: 启动传输 */
#define REG_DMA_CH_DIRECTION    0x004   /* 地址类型与跨设备标志 */
#define REG_DMA_CH_MMU_ADDR     0x010   /* MMU 转换模式 */
#define REG_DMA_CH_INTR_IMSK    0x0C4   /* 中断掩码 */
#define REG_DMA_CH_INTR_RAW     0x0C8   /* 原始中断（写 1 清零） */
#define REG_DMA_CH_INTR_STATUS  0x0CC   /* 屏蔽后中断状态 */
#define REG_DMA_CH_STATUS       0x0D0   /* bit0: 通道忙 */
#define REG_DMA_CH_DESC_OPT     0x400   /* bit0=中断使能, bit1=链式使能 */
#define REG_DMA_CH_ACNT         0x404   /* 字节计数 */
#define REG_DMA_CH_SAR_L        0x408   /* 源地址低 32 位 */
#define REG_DMA_CH_SAR_H        0x40C   /* 源地址高 32 位 */
#define REG_DMA_CH_DAR_L        0x410   /* 目的地址低 32 位 */
#define REG_DMA_CH_DAR_H        0x414   /* 目的地址高 32 位 */
#define REG_DMA_CH_LAR_L        0x418   /* 下一描述符地址低 32 位 */
#define REG_DMA_CH_LAR_H        0x41C   /* 下一描述符地址高 32 位 */
```

**链式描述符结构体（28 字节，packed）：**

```c
struct dma_ch_desc {
    uint32_t desc_op;   /* BIT(0)=中断使能, BIT(1)=链式使能 */
    uint32_t cnt;       /* 本段字节计数 */
    union { uint64_t reg; struct { uint32_t lsb, msb; }; } sar;
    union { uint64_t reg; struct { uint32_t lsb, msb; }; } dar;
    union { uint64_t reg; struct { uint32_t lsb, msb; }; } lar; /* 下一描述符地址 */
} __packed;
```

**中断状态位：**

```c
#define DMA_CH_INTR_BIT_DONE            BIT(0)  /* 传输成功 */
#define DMA_CH_INTR_BIT_ERR_DATA        BIT(1)  /* 数据错误 */
#define DMA_CH_INTR_BIT_ERR_DESC_READ   BIT(2)  /* 描述符读取失败 */
#define DMA_CH_INTR_BIT_ERR_CFG         BIT(3)  /* 配置错误 */
#define DMA_CH_INTR_BIT_ERR_DUMMY_READ  BIT(4)  /* 地址合法性检查失败 */
```

### 3.3 `driver/mt-emu-mtdma-core.h` — DMA Engine 核心结构

```c
#define MTDMA_MAX_WR_CH 64   /* 最大写通道数 */
#define MTDMA_MAX_RD_CH 64   /* 最大读通道数 */

enum dma_transfer_direction {
    DMA_MEM_TO_MEM = 0,   /* H2H：主机到主机 */
    DMA_MEM_TO_DEV = 1,   /* H2D：主机到设备（写） */
    DMA_DEV_TO_MEM = 2,   /* D2H：设备到主机（读） */
    DMA_DEV_TO_DEV = 3,   /* D2D：设备到设备 */
    DMA_TRANS_NONE,
};

/* 描述符存放位置 */
#define DMA_DESC_IN_DEVICE  0   /* 描述符存放在设备 DDR（推荐） */
#define DMA_DESC_IN_HOST    1   /* 描述符存放在主机内存 */
```

---

## 4. Makefile 生成规范

AI 工具生成 `driver/Makefile` 时需遵循以下模板，并将配置项替换为第 1 节的值：

```makefile
export CPATH:=${PWD}/lib

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

# ── 可配置参数（对应 skills/driver_skill.md 第 1 节）──
VF_NUM      ?= {VF_NUM}           # 虚函数数量（0~60）
DMA_RESV_MEM ?= {DMA_RESV_MEM}   # 1=保留内存模式
DDR_SZ_GB   ?= {DDR_SZ_GB}       # DDR 容量（GB）
ROM_ENABLE  ?= {ROM_ENABLE}       # 1=启用扩展 ROM

# ── 编译标志 ──
ccflags-y += -DVF_NUM=$(VF_NUM)
ccflags-y += -DDMA_RESV_MEM=$(DMA_RESV_MEM)
ccflags-y += -DDDR_SZ_GB=$(DDR_SZ_GB)
# 调试开关（根据 ENABLE_DEBUG / ENABLE_VERBOSE_DEBUG 决定是否加入）
ccflags-y += -DDEBUG
ccflags-y += -DVERBOSE_DEBUG
ccflags-y += -DCONFIG_DEBUG_FS
ccflags-y += -DUSE_PLATFORM_DEVICE
ccflags-y += -g

# ── 模块目标 ──
obj-m += mt_emu_gpu.o
obj-m += mt_emu_apu.o
obj-m += mt_emu_vgpu.o
obj-m += mt_emu_mtdma.o

mt_emu_gpu-m += mt-emu-gpu.o mt-emu-ioctl.o mt-emu-dmabuf.o mt-emu-intr.o \
                mt-emu-mtdma-bare.o mt-emu-mtdma-test.o eata_api.o mmu_init_pagetable.o

mt_emu_apu-m += mt-emu-apu.o mt-emu-ioctl.o mt-emu-mtdma-bare.o mt-emu-mtdma-test.o

mt_emu_vgpu-m += mt-emu-ioctl.o mt-emu-vgpu.o mt-emu-mtdma-bare.o mt-emu-mtdma-test.o

mt_emu_mtdma-m += mt-emu-mtdma-core.o virt-dma.o

all: modules

modules:
	sed -i 's/\(VF_NUM[[:space:]]\+\)[0-9]\+/\1$(VF_NUM)/g' mt-emu-drv.h
	$(MAKE) -C $(KERNELDIR) M=$$PWD modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$$PWD modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod \
		.tmp_versions modules.order Module.symvers .cache.mk \
		pcie_qy_test *.tmp *.log
```

---

## 5. 脚本生成模板

### 5.1 `scripts/build.sh` — 构建脚本

> 注：仓库中现有文件名为 `bulid.sh`（历史拼写错误），AI 生成新脚本时应使用正确拼写 `build.sh`。

```bash
#!/bin/bash
# MT EMU PCIe 驱动 + 测试套件构建脚本
# 配置参数来源：skills/driver_skill.md 第 1 节

set -e
LOG_FILE="build.log"

export EMU_DRV_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# ── 系统依赖 ──
sudo apt install -y build-essential cmake
sudo apt install -y linux-source
sudo apt install -y linux-headers-$(uname -r)

# ── 编译驱动模块 ──
# 参数说明：
#   VF_NUM      = {VF_NUM}        虚函数数量
#   DMA_RESV_MEM= {DMA_RESV_MEM}  DMA 保留内存模式
#   DDR_SZ_GB   = {DDR_SZ_GB}     DDR 容量 GB
#   ROM_ENABLE  = {ROM_ENABLE}    扩展 ROM 支持
cd "${EMU_DRV_DIR}/driver"
make clean && make VF_NUM={VF_NUM} DMA_RESV_MEM={DMA_RESV_MEM} \
    DDR_SZ_GB={DDR_SZ_GB} ROM_ENABLE={ROM_ENABLE}

# ── 编译测试套件 ──
rm -rf "${EMU_DRV_DIR}/test/build"
mkdir -p "${EMU_DRV_DIR}/test/build"
cd "${EMU_DRV_DIR}/test/build"
cmake ..
make

cd "${EMU_DRV_DIR}"
echo "✅ 构建完成"
```

### 5.2 `scripts/install.sh` — 驱动安装脚本

```bash
#!/bin/bash
# MT EMU PCIe 驱动安装脚本
# ⚠️  加载顺序不可颠倒：mtdma → gpu → apu → vgpu

export EMU_DRV_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# ── 系统日志级别与挂起超时 ──
echo 8 > /proc/sys/kernel/printk
echo 0 > /proc/sys/kernel/hung_task_timeout_secs

# ── 启用动态调试（可按需注释）──
echo 'module mt_emu_gpu +p' > /sys/kernel/debug/dynamic_debug/control

# ── 卸载旧模块（忽略未加载的报错）──
sudo rmmod mt_emu_vgpu  2>/dev/null || true
sudo rmmod mt_emu_apu   2>/dev/null || true
sudo rmmod mt_emu_gpu   2>/dev/null || true
sudo rmmod mt_emu_mtdma 2>/dev/null || true

# ── 加载新模块（顺序固定）──
sudo insmod "${EMU_DRV_DIR}/driver/mt_emu_mtdma.ko"
sudo insmod "${EMU_DRV_DIR}/driver/mt_emu_gpu.ko"
sudo insmod "${EMU_DRV_DIR}/driver/mt_emu_apu.ko"
sudo insmod "${EMU_DRV_DIR}/driver/mt_emu_vgpu.ko"

# ── 清空 dmesg 缓冲并切换到测试目录 ──
sudo dmesg -c
cd "${EMU_DRV_DIR}/test/build"

echo "✅ 驱动安装完成"
echo "设备节点："
ls -la /dev/mt_emu* 2>/dev/null || echo "  （未发现设备节点，请检查硬件连接）"
```

### 5.3 `scripts/clean.sh` — 清理脚本

```bash
#!/bin/bash
# MT EMU PCIe 驱动清理脚本

export EMU_DRV_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# ── 卸载模块 ──
sudo rmmod mt_emu_vgpu  2>/dev/null || true
sudo rmmod mt_emu_apu   2>/dev/null || true
sudo rmmod mt_emu_gpu   2>/dev/null || true
sudo rmmod mt_emu_mtdma 2>/dev/null || true

# ── 清理编译产物 ──
cd "${EMU_DRV_DIR}/driver" && make clean
rm -rf "${EMU_DRV_DIR}/test/build"

echo "✅ 清理完成"
```

### 5.4 `scripts/test_sanity.sh` — PCIe 速度切换与健全测试脚本

```bash
#!/bin/bash
# MT EMU PCIe 速度健全测试脚本
# 依赖：已安装驱动，已编译测试套件
# 配置参数：
#   PCIE_ROOT_BUS   = {PCIE_ROOT_BUS}      RC 端口
#   PCIE_EP_BUS_GPU = {PCIE_EP_BUS_GPU}    EP GPU 设备
#   PCIE_EP_BUS_APU = {PCIE_EP_BUS_APU}    EP APU 设备

MYDIR="$(cd "$(dirname "$0")/.." && pwd)"
RC="{PCIE_ROOT_BUS}"
EP_GPU="{PCIE_EP_BUS_GPU}"
EP_APU="{PCIE_EP_BUS_APU}"

run_sanity() {
    local gen=$1
    local log="sanity_gen${gen}.log"
    echo "=== PCIe Gen${gen} 健全测试 ===" | tee "${log}"
    setpci -s "${EP_GPU}" cap10+30.b="${gen}"
    setpci -s "${RC}" 3e.b=42
    setpci -s "${RC}" 3e.b=2
    sleep 10
    lspci -vvvs "${EP_GPU}" | grep Lnk | tee -a "${log}"
    echo 1 > /sys/bus/pci/devices/0000:${EP_GPU//:\//.}/remove  2>/dev/null || true
    echo 1 > /sys/bus/pci/devices/0000:${EP_APU//:\//.}/remove  2>/dev/null || true
    echo 1 > /sys/bus/pci/devices/0000:${RC//:\//.}/rescan      2>/dev/null || true
    "${MYDIR}/test/build/test" "[sanity]" | tee -a "${log}"
}

for gen in 4 3 2 1; do
    run_sanity ${gen}
done

echo "✅ 所有速度等级健全测试完成"
```

### 5.5 `scripts/test_stress.sh` — 24 小时压力测试脚本

```bash
#!/bin/bash
# MT EMU PCIe DMA 24 小时压力测试脚本
# 同时启动 3 个并行实例，分别写入独立日志

MYDIR="$(cd "$(dirname "$0")/.." && pwd)"
TEST_BIN="${MYDIR}/test/build/test"

echo "=== 开始 24 小时压力测试 ==="
"${TEST_BIN}" stress_24h_bare > "${MYDIR}/stress0.log" 2>&1 &
"${TEST_BIN}" stress_24h_bare > "${MYDIR}/stress1.log" 2>&1 &
"${TEST_BIN}" stress_24h_bare > "${MYDIR}/stress2.log" 2>&1 &

wait
echo "✅ 压力测试完成，日志：stress0.log / stress1.log / stress2.log"
```

---

## 6. 驱动核心源文件生成规范

AI 工具在生成或修改驱动源文件时须遵循以下规范。

### 6.1 `driver/mt-emu-gpu.c` — GPU PF0 驱动

**必须实现的接口：**

| 接口 | 说明 |
|------|------|
| `mt_emu_gpu_probe()` | PCI 设备发现：BAR 映射、MSI-X 初始化、注册 `/dev/mt_emu_gpu` |
| `mt_emu_gpu_remove()` | 设备移除时的清理 |
| `mt_emu_gpu_open()` | 文件打开 |
| `mt_emu_gpu_release()` | 文件关闭 |
| `mt_emu_gpu_ioctl()` | ioctl 命令分发（转至 `mt-emu-ioctl.c`） |
| `mt_emu_gpu_mmap()` | BAR 内存映射（用于用户态直接访问） |

**PCI 设备 ID 表模板（根据配置项 `ACTIVE_GPU_DEVICE_ID`）：**

```c
static const struct pci_device_id mt_emu_gpu_ids[] = {
    { PCI_DEVICE(PCI_VENDOR_ID_MT, PCI_DEVICE_ID_MT_QY_GPU)  },
    { PCI_DEVICE(PCI_VENDOR_ID_MT, PCI_DEVICE_ID_MT_QY2_GPU) },
    { PCI_DEVICE(PCI_VENDOR_ID_MT, PCI_DEVICE_ID_MT_PH1S_GPU) },
    { PCI_DEVICE(PCI_VENDOR_ID_MT, PCI_DEVICE_ID_MT_HS_GPU)  },
    { PCI_DEVICE(PCI_VENDOR_ID_MT, PCI_DEVICE_ID_MT_LS_GPU)  },
    { 0 }
};
MODULE_DEVICE_TABLE(pci, mt_emu_gpu_ids);
```

### 6.2 `driver/mt-emu-ioctl.c` — ioctl 命令处理

**命令路由表：**

```c
long mt_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
    case MT_IOCTL_BAR_RW:         return mt_ioctl_bar_rw(file, arg);
    case MT_IOCTL_CFG_RW:         return mt_ioctl_cfg_rw(file, arg);
    case MT_IOCTL_WAIT_INT:       return mt_ioctl_wait_int(file, arg);
    case MT_IOCTL_TRIG_INT:       return mt_ioctl_trig_int(file, arg);
    case MT_IOCTL_IRQ_INIT:       return mt_ioctl_irq_init(file, arg);
    case MT_IOCTL_DMAISR_SET:     return mt_ioctl_dmaisr_set(file, arg);
    case MT_IOCTL_MTDMA_BARE_RW:  return mt_ioctl_mtdma_bare_rw(file, arg);
    case MT_IOCTL_MTDMA_RW:       return mt_ioctl_mtdma_rw(file, arg);
    default:                      return -ENOIOCTLCMD;
    }
}
```

### 6.3 `driver/mt-emu-mtdma-bare.c` — 裸机 DMA 传输

**核心函数签名（AI 生成时保持此接口不变）：**

```c
/* 执行一次裸机 DMA 传输，返回 0 成功 */
int dma_bare_xfer(struct dma_bare *bare,
                  struct dma_bare_rw *rw_info);

/* DMA 完成中断处理 */
irqreturn_t dma_bare_isr(int irq, void *dev_id);

/* 初始化通道 */
int dma_bare_ch_init(struct dma_bare *bare,
                     struct mtdma_chan_info *info);
```

**执行路径（用于调试）：**

```
用户态 pcief_dma_bare_xfer()
  └─ ioctl(fd, MT_IOCTL_MTDMA_BARE_RW, ...)
       └─ mt_test_ioctl()          [mt-emu-ioctl.c]
            └─ dma_bare_xfer()     [mt-emu-mtdma-bare.c]
                 ├─ 写描述符到寄存器或设备内存链表
                 ├─ SET_CH_32(REG_DMA_CH_ENABLE, 1)  ← 启动 DMA
                 └─ wait_for_completion_timeout()     ← 等待完成中断
                      ↑
                 MSI 中断 → dma_bare_isr()
                      └─ complete(&bare_ch->int_done)
```

---

## 7. 设备节点与文件操作

| 设备节点 | 对应模块 | 用途 |
|---------|---------|------|
| `/dev/mt_emu_gpu`       | mt_emu_gpu.ko  | PF0 GPU（裸机 DMA 主入口） |
| `/dev/mt_emu_apu`       | mt_emu_apu.ko  | PF1 APU |
| `/dev/mt_emu_vgpu0`~`59`| mt_emu_vgpu.ko | VF 虚拟功能（数量由 VF_NUM 控制） |
| `/dev/mt_emu_dmabuf`    | mt_emu_gpu.ko  | 主机侧 DMA 缓冲区管理 |

---

## 8. 切换项目或设备的操作流程

当需要适配新设备或新项目时，按以下顺序操作：

1. **修改第 1 节的配置项**（YAML 块），更新 `ACTIVE_GPU_DEVICE_ID`、`VF_NUM`、`DDR_SZ_GB` 等。
2. **由 AI 重新生成 `driver/Makefile`**（参考第 4 节模板，替换 `{...}` 占位符）。
3. **由 AI 更新 `driver/mt-emu-drv.h`** 中的 `PCI_DEVICE_ID_MT_*` 宏，确保新设备 ID 已添加。
4. **由 AI 更新 `driver/mt-emu-gpu.c`** 中的 `pci_device_id` 表。
5. **由 AI 更新 `scripts/bulid.sh`**（替换 `{...}` 占位符）。
6. **由 AI 更新 `scripts/install.sh` / `scripts/test_sanity.sh`**（替换总线地址）。
7. 执行 `bash scripts/bulid.sh` 编译，`bash scripts/install.sh` 加载。

---

## 9. 常见故障排查

| 现象 | 可能原因 | 排查命令 |
|------|----------|---------|
| `insmod` 失败，`-1 (ENODEV)` | 设备 ID 不在驱动支持表中 | `lspci -n \| grep 1ed5` |
| 模块加载乱序导致符号找不到 | 未按 mtdma→gpu→apu→vgpu 顺序 | `lsmod \| grep mt_emu` |
| `/dev/mt_emu_gpu` 不存在 | GPU 模块未正常 probe | `dmesg \| grep mt_emu` |
| DMA 传输超时 | MSI-X 未初始化或中断未到达 | `dmesg \| grep -E "mtdma\|DMA int"` |
| 传输完成但数据错误 | DDR 地址越界或 desc_cnt 填错 | 检查 `dma_bare_rw.sar/dar` 是否在有效范围 |
| VF 设备节点缺失 | VF_NUM=0 或 SRIOV 未使能 | `cat /sys/bus/pci/devices/*/sriov_numvfs` |
