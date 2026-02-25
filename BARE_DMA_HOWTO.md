# 裸机模式 DMA 数据搬运操作指南

本文档说明如何使用裸机模式（Bare Metal）通过 MTDMA 控制器完成一次 PCIe DMA 数据搬运。
裸机模式直接操作 DMA 寄存器，绕过 Linux DMA Engine 框架，适合验证和性能测试场景。

---

## 一、前提条件

| 项目 | 说明 |
|------|------|
| 内核版本 | Linux 5.x+ |
| 编译工具 | gcc/g++（支持 C++11）、make、cmake |
| PCIe 设备 | MT EMU GPU/APU（Vendor ID `0x1ed5`） |
| 保留内存 | 在 grub.cfg 中配置 `mem=32G memmap=32G$32G`（DMA 大缓冲区，见 README） |

---

## 二、第一步：编译驱动

```bash
cd <repo_root>/driver
make
```

编译成功后，生成以下 `.ko` 文件：
- `mt_emu_mtdma.ko`
- `mt_emu_gpu.ko`
- `mt_emu_apu.ko`
- `mt_emu_vgpu.ko`

---

## 三、第二步：加载驱动

```bash
cd <repo_root>

# 卸载旧版本（若已加载）
sudo rmmod mt_emu_vgpu
sudo rmmod mt_emu_apu
sudo rmmod mt_emu_gpu
sudo rmmod mt_emu_mtdma

# 加载新版本（顺序不能颠倒）
sudo insmod driver/mt_emu_mtdma.ko
sudo insmod driver/mt_emu_gpu.ko
sudo insmod driver/mt_emu_apu.ko
sudo insmod driver/mt_emu_vgpu.ko
```

加载成功后会在 `/dev/` 下创建设备节点：

| 设备节点 | 对应 PCIe 功能 |
|----------|---------------|
| `/dev/mt_emu_gpu` | PF0 GPU（主设备，裸机模式主要使用此节点） |
| `/dev/mt_emu_vgpu0` ~ `/dev/mt_emu_vgpu59` | VF 虚拟功能 |
| `/dev/mt_emu_dmabuf` | DMA 缓冲区管理 |

也可直接使用仓库提供的安装脚本：

```bash
cd <repo_root>
bash scripts/install.sh
```

---

## 四、第三步：编译测试程序

```bash
cd <repo_root>/test
mkdir -p build && cd build
cmake ..
make
```

编译成功后生成可执行文件 `test`（位于 `build/` 目录）。

---

## 五、第四步：执行裸机 DMA 传输

### 5.1 使用内置测试用例（推荐）

测试程序基于 Catch2 框架，通过 `-t` 参数选择测试用例。

#### 5.1.1 单描述符模式（最简单，一次传输一块连续数据）

```bash
# Host→Device（写设备 DDR）+ Device→Host（读设备 DDR），传输大小 512KB，通道 0
./test -t "sanity_dma_bare_single_s"
```

等效于调用：
```c
// 单描述符，通道 0，设备地址 0x0，大小 512KB
pcief_dma_bare_xfer(DMA_MEM_TO_DEV, DMA_DESC_IN_DEVICE,
                    /*desc_cnt*/0, /*block_cnt*/0, /*ch_num*/0,
                    /*sar*/host_phys_addr, /*dar*/0x0,
                    /*size*/512*1024, /*timeout_ms*/cal_timeout(512*1024));
pcief_dma_bare_xfer(DMA_DEV_TO_MEM, DMA_DESC_IN_DEVICE,
                    0, 0, 0,
                    0x0, host_phys_addr,
                    512*1024, cal_timeout(512*1024));
```

#### 5.1.2 链式描述符模式（分散传输，单通道多段数据）

```bash
# 32 个描述符，总大小 32×4KB=128KB
./test -t "sanity_dma_bare_chain_ddr"
```

#### 5.1.3 块模式（多块链式传输）

```bash
# 32 块，每块 8 个描述符
./test -t "sanity_dma_bare_block_ddr"
```

#### 5.1.4 按标签批量运行所有裸机 DMA 用例

```bash
# 运行所有 [mtdma1] 标签的用例（包含单/链/块模式对 DDR 的读写测试）
./test -t "[mtdma1]"

# 运行最基础的单描述符验证
./test -t "[mtdma0]"
```

---

### 5.2 在自定义代码中调用裸机 DMA 接口

核心 API 函数（定义在 `test/lib/mt_pcie_f.h`）：

```c
int pcief_dma_bare_xfer(
    uint32_t data_direction,  // 数据搬运方向（见下表）
    uint32_t desc_direction,  // 描述符存放位置（见下表）
    uint32_t desc_cnt,        // 链式描述符数量（0=单描述符模式；N-1=共N个描述符）
    uint32_t block_cnt,       // 块数量（0=不使用块模式）
    uint32_t ch_num,          // DMA 通道号（0~63，PF 通道；或 VF 编号）
    uint64_t sar,             // 源地址（Source Address）
    uint64_t dar,             // 目的地址（Destination Address）
    uint32_t size,            // 传输总字节数
    uint32_t timeout_ms       // 超时时间（毫秒）
);
// 返回值：0 = 成功，非 0 = 失败
```

**`data_direction` 取值：**

| 值 | 宏名称 | 含义 | SAR 类型 | DAR 类型 |
|----|--------|------|----------|----------|
| 0 | `DMA_MEM_TO_MEM` | 主机内存→主机内存（H2H） | 主机物理地址 | 主机物理地址 |
| 1 | `DMA_MEM_TO_DEV` | 主机内存→设备 DDR（H2D，写） | 主机物理地址 | 设备本地地址 |
| 2 | `DMA_DEV_TO_MEM` | 设备 DDR→主机内存（D2H，读） | 设备本地地址 | 主机物理地址 |
| 3 | `DMA_DEV_TO_DEV` | 设备 DDR→设备 DDR（D2D） | 设备本地地址 | 设备本地地址 |

**`desc_direction` 取值：**

| 值 | 宏名称 | 含义 |
|----|--------|------|
| 0 | `DMA_DESC_IN_DEVICE` | 描述符链表存放在设备内存（通过 MMIO 写入，推荐） |
| 1 | `DMA_DESC_IN_HOST` | 描述符链表存放在主机内存（系统内存） |

**完整示例：Host 向设备 DDR 0x0 写入 1MB 数据（单描述符）**

```c
#include "mt_pcie_f.h"
#include "mt-emu-drv.h"

int main() {
    // 1. 初始化 PCIe 库（打开设备节点）
    pcief_init();

    // 2. 获取主机 DMA 缓冲区物理地址（通过 dmabuf 设备）
    uint64_t host_paddr, host_maddr, mtdma_size;
    pcief_get_mtdma_info(&host_paddr, &host_maddr, &mtdma_size);

    uint32_t xfer_size = 1 * 1024 * 1024; // 1MB

    // 3. 准备源数据（写入主机缓冲区的虚拟地址映射）
    volatile uint32_t *host_buf = (volatile uint32_t *)host_maddr;
    for (uint32_t i = 0; i < xfer_size / 4; i++)
        host_buf[i] = i;

    // 4. Host→Device：将主机缓冲区数据搬运到设备 DDR 地址 0x0
    int ret = pcief_dma_bare_xfer(
        DMA_MEM_TO_DEV,       // 写设备
        DMA_DESC_IN_DEVICE,   // 描述符在设备内存
        0,                    // 单描述符（desc_cnt=0）
        0,                    // 不使用块模式
        0,                    // 通道 0
        host_paddr,           // SAR：主机物理地址
        0x0,                  // DAR：设备 DDR 偏移 0x0
        xfer_size,            // 1MB
        5000                  // 5 秒超时
    );
    printf("H2D result: %s\n", ret == 0 ? "OK" : "FAIL");

    // 5. Device→Host：从设备 DDR 地址 0x0 读回数据
    ret = pcief_dma_bare_xfer(
        DMA_DEV_TO_MEM,       // 读设备
        DMA_DESC_IN_DEVICE,
        0, 0, 0,
        0x0,                  // SAR：设备 DDR 地址
        host_paddr + xfer_size, // DAR：主机物理地址（另一块区域）
        xfer_size,
        5000
    );
    printf("D2H result: %s\n", ret == 0 ? "OK" : "FAIL");

    return 0;
}
```

**链式描述符示例：32 个描述符，每段 4KB，总计 128KB**

```c
uint32_t desc_n   = 32;                    // 共 32 个描述符
uint32_t seg_size = 4 * 1024;             // 每段 4KB
uint32_t total    = desc_n * seg_size;    // 总 128KB

int ret = pcief_dma_bare_xfer(
    DMA_MEM_TO_DEV,
    DMA_DESC_IN_DEVICE,
    desc_n - 1,   // desc_cnt = N-1（共 N 个描述符）
    0,            // block_cnt = 0
    0,            // 通道 0
    host_paddr,   // SAR
    0x0,          // DAR
    total,        // 总大小
    5000
);
```

---

## 六、ioctl 底层调用参考

如果不使用 `pcief_dma_bare_xfer()` 封装函数，可以直接调用 ioctl（需要包含 `mt-emu-drv.h`）：

```c
#include <sys/ioctl.h>
#include <fcntl.h>
#include "mt-emu-drv.h"

// 打开设备
int fd = open("/dev/mt_emu_gpu", O_RDWR | O_SYNC);

// 构建参数结构体
struct {
    struct mt_emu_param param;
    struct dma_bare_rw  rw;
} req = {};

req.param.d0            = sizeof(struct dma_bare_rw);
req.rw.data_direction   = DMA_MEM_TO_DEV;
req.rw.desc_direction   = DMA_DESC_IN_DEVICE;
req.rw.desc_cnt         = 0;       // 单描述符
req.rw.block_cnt        = 0;
req.rw.ch_num           = 0;       // 通道 0
req.rw.sar              = host_phys_addr;
req.rw.dar              = 0x0;     // 设备 DDR 偏移
req.rw.size             = 1024 * 1024;
req.rw.timeout_ms       = 5000;

int ret = ioctl(fd, MT_IOCTL_MTDMA_BARE_RW, &req.param);
// req.param.b0 == 1 表示传输成功

close(fd);
```

---

## 七、常见问题排查

| 现象 | 可能原因 | 排查方法 |
|------|----------|----------|
| `ioctl` 返回 `-1`（`ENOENT`） | 设备节点不存在 | 检查 `ls /dev/mt_emu*`，确认驱动已加载 |
| `ioctl` 返回 `-1`（`EBUSY`） | 通道被占用 | 等待其他传输完成，或换用其他通道号 |
| 传输超时（返回 0 但 `b0 != 1`） | 中断未到达 | 检查 MSI/MSI-X 是否正确初始化（`dmesg`），确认 IRQ 路由 |
| DMA 错误中断 | 地址越界或地址类型配置错误 | 检查 SAR/DAR 的合法范围；`DMA_DEV_TO_MEM` 的 SAR 必须是设备地址 |
| 数据校验失败 | 描述符链未完整写入 | 确认 `desc_cnt` 值正确（链式模式：总描述符数 = `desc_cnt + 1`） |

查看内核日志：
```bash
sudo dmesg | grep -E "mtdma|DMA int|dma_bare"
```

---

## 八、内核侧执行路径快速参考

```
用户态 pcief_dma_bare_xfer()
  └─ ioctl(fd, MT_IOCTL_MTDMA_BARE_RW, ...)     [用户态]
       └─ mt_test_ioctl() / case MT_IOCTL_MTDMA_BARE_RW  [内核 mt-emu-ioctl.c]
            └─ dma_bare_xfer(bare_ch, ...)               [内核 mt-emu-mtdma-bare.c]
                 ├─ 写描述符到寄存器/链表内存
                 ├─ SET_CH_32(REG_DMA_CH_ENABLE, ENABLE)  ← 启动 DMA
                 └─ wait_for_completion_timeout()          ← 等待中断
                      ↑
                 MSI 中断 → dma_bare_isr()
                      └─ complete(&bare_ch->int_done)
```
