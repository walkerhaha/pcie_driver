# Skill：PCIe & DMA 测试集重建

## 1. 目标

基于当前仓库 `test/` 的逻辑，沉淀一套**只靠文档也能重建**的测试集 skill。重点不是抄现有测试代码，而是明确：

- 测试入口如何组织
- 测试 API 如何分层
- 测试标签如何分组
- DMA、基础寄存器、中断、压力测试如何参数化

---

## 2. 测试集总结构

| 路径 | 职责 |
|---|---|
| `test/src/main.cc` | Catch2 入口，负责 `pcief_init()` / `pcief_uninit()` |
| `test/src/test_thr.h` | 线程化测试参数结构与启动函数声明 |
| `test/lib/mt_pcie_f.h` | 用户态 API 头，关键接口总表 |
| `test/lib/mt_pcie_f.c` | 用户态 API 实现，封装 `/dev/*` + IOCTL |
| `test/src/base.cc` | BAR、ROM、IPC、配置空间类测试 |
| `test/src/intr.cc` | Legacy/MSI/MSI-X 与 DMA 中断测试 |
| `test/src/mthdma.cc` | bare DMA / DMA engine / MMU / 性能 / 错误注入 |
| `test/src/stress.cc` | 长稳、扫描、压力回归 |

---

## 3. 测试入口契约

来源：`test/src/main.cc`

入口行为固定为：

1. 创建 `Catch::Session`
2. 扩展 `-g/--cfg` 参数
3. `session.applyCommandLine(argc, argv)`
4. `pcief_init()`
5. `session.run()`
6. `pcief_uninit()`

如果未来项目重建测试集，入口必须保留这个顺序，尤其是：

- **测试前统一初始化设备发现**
- **测试后统一清理**

---

## 4. 必须显式列出的关键测试接口

以下接口是测试集 skill 的“公开面”。

### 4.1 初始化与发现接口

来源：`test/lib/mt_pcie_f.h`

```c
void pcief_init();
void pcief_uninit();
struct pcief_bar *pcief_get_barinfo(uint8_t fun, uint8_t bar);
int pcief_get_vf__num();
```

### 4.2 BAR / CFG / ROM 访问接口

```c
int pcief_write(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data);
int pcief_read(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data);
int pcief_cfg_write(uint8_t fun, uint32_t offset, uint32_t len, void* data);
int pcief_cfg_read(uint8_t fun, uint32_t offset, uint32_t len, void* data);
int pcief_read_exp_rom(uint32_t len, void* data);
```

便捷寄存器接口：

```c
uint32_t pcief_greg_u32(uint8_t fun, uint8_t bar, uint64_t address);
uint64_t pcief_greg_u64(uint8_t fun, uint8_t bar, uint64_t address);
uint8_t  pcief_greg_u8(uint8_t fun, uint8_t bar, uint64_t address);
uint16_t pcief_greg_u16(uint8_t fun, uint8_t bar, uint64_t address);
void pcief_sreg_u32(uint8_t fun, uint8_t bar, uint64_t address, uint32_t value);
void pcief_sreg_u64(uint8_t fun, uint8_t bar, uint64_t address, uint64_t value);
void pcief_sreg_u8(uint8_t fun, uint8_t bar, uint64_t address, uint8_t value);
void pcief_sreg_u16(uint8_t fun, uint8_t bar, uint64_t address, uint16_t value);
```

### 4.3 DMA 缓冲区接口

```c
int  pcief_dmabuf_write(uint64_t offset, uint32_t len, void* data);
int  pcief_dmabuf_read(uint64_t offset, uint32_t len, void* data);
long pcief_dmabuf_malloc(uint64_t len);
void pcief_dmabuf_free(long addr);
```

### 4.4 中断与 IPC 接口

```c
int pcief_wait_int(uint8_t fun, int irq, uint32_t* done, uint32_t timeout_ms);
int pcief_trig_int(uint8_t fun, int irq, uint32_t* done);
int pcief_tgt_cmd(uint8_t target, uint32_t* done, uint32_t timeout_ms);
int pcief_irq_init(uint8_t fun, uint8_t type, uint8_t test_mode);
int pcief_dmaisr_set(uint8_t fun, uint8_t dmabare);
```

### 4.5 bare DMA 接口

```c
int pcief_dma_bare_xfer(uint32_t data_direction, uint32_t desc_direction,
                        uint32_t desc_cnt, uint32_t block_cnt, uint32_t ch_num,
                        uint64_t sar, uint64_t dar, uint32_t size, uint32_t timeout_ms);
```

### 4.6 DMA engine 接口

```c
void* pcief_mtdma_engine_malloc(uint32_t size);
void  pcief_mtdma_engine_free(void *ptr);
int   pcief_mtdma_engine_start(int fun, struct mtdma_rw *info, void* rw_buf, uint32_t* done);
```

### 4.7 地址辅助接口

```c
uint64_t pcief_vf_base_addr(int vf);
int pcief_mtdma_pf_ch_num();
long time_get_ms();
unsigned int make_crc(unsigned int crc, unsigned char *string, unsigned int size);
```

---

## 5. 线程化参数结构

来源：`test/src/test_thr.h`

### 5.1 DMA engine 参数

```c
struct mtdma_engine_test_data {
    uint64_t laddr;
    void *raddr_v;
    uint32_t size;
    int ch;
    int cnt;
    int timeout_ms;
    int ret;
    volatile int run;
};
```

### 5.2 bare DMA 参数

```c
struct dma_bare_test_data {
    uint64_t device_sar;
    uint64_t device_dar;
    uint64_t len;
    uint32_t data_direction_bits;
    uint32_t desc_direction;
    uint32_t desc_cnt;
    uint32_t block_cnt;
    uint32_t ch_num;
    uint32_t cnt;
    uint32_t rw;
    uint32_t timeout_ms;
    uint32_t offset;
    uint32_t ret;
    volatile uint32_t run;
};
```

### 5.3 启动函数

```c
int start_thr_rand_BAR02(struct pcie_test_thrd* thr, int fun, int bar, uint64_t laddr, int len, int cnt);
int start_thr_rand_mtdma_engine(struct pcie_test_thrd* thr, int ch, uint64_t laddr, void *raddr_v, int size, int cnt, int timeout_ms);
int host_mem_wr(uint64_t len);
int start_thr_rand_dma_bare(struct pcie_test_thrd* thr, uint32_t data_direction_bits,
                            uint32_t desc_direction, uint32_t desc_cnt, uint32_t block_cnt,
                            uint32_t ch_num, uint64_t device_sar, uint64_t device_dar,
                            uint64_t len, int cnt, int timeout_ms, int offset);
int start_thr_ipc(struct pcie_test_thrd* thr, int cnt);
```

---

## 6. bare DMA 参数化规则

来源：`test/src/test_thr.h`、`test/src/mthdma.cc`

### 6.1 三种描述符模式

| 模式 | `desc_cnt` | `block_cnt` | 含义 |
|---|---:|---:|---|
| 单描述符 | `0` | `0` | 寄存器区直接描述符 |
| 链式 | `N - 1` | `0` | N 个描述符串起来 |
| 块模式 | `D - 1` | `B` | 每块 D 个描述符，共 B 块 |

### 6.2 超时规则

来源：`test/src/test_thr.h`

```c
static int cal_timeout(uint64_t size) {
    return ((size*1000)/MTDMA_EDK_SPEED);
}
```

说明：

- 基准速度常量：`MTDMA_EDK_SPEED = 2*1024*1024`
- 大链或块模式通常还要乘以额外系数

### 6.3 多方向位掩码

典型写法：

```c
BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM) | BIT(DMA_DEV_TO_DEV)
```

也可加入：

```c
BIT(DMA_MEM_TO_MEM)
```

### 6.4 多通道规则

来源：`test/src/mthdma.cc`

`dma_bare_simple_test()` 会自动：

1. 从 `ch_start_num` 开始逐通道启动
2. 对每个通道把 `device_sar += size`
3. 对每个通道把 `device_dar += size`
4. `pthread_join` 后对每个线程 `REQUIRE(ret == 0)`

---

## 7. 关键测试辅助逻辑

来源：`test/src/mthdma.cc`

```c++
static void dma_bare_move(uint32_t data_direction, uint64_t addr, uint32_t size);
static void dma_bare_simple_test(uint32_t ch_start_num, uint32_t ch_cnt,
                                 uint32_t data_direction_bits, uint32_t desc_direction,
                                 uint32_t desc_cnt, uint32_t block_cnt,
                                 uint64_t device_sar, uint64_t device_dar,
                                 uint64_t size, int cnt, int offset);
```

这两个辅助函数定义了测试集的最短闭环：

- 组织请求参数
- 调 `start_thr_rand_dma_bare()`
- 等待线程
- 用 `REQUIRE()` 做强断言

---

## 8. 测试标签体系

来源：`test/src/base.cc`、`test/src/intr.cc`、`test/src/mthdma.cc`、`test/src/stress.cc`

### 8.1 DMA 标签

- `[mtdma]`
- `[mtdma0]`
- `[mtdma1]`
- `[mtdma2]`
- `[mtdma_mmu]`

### 8.2 基础功能标签

- `[base]`
- `[ph1s_base]`

### 8.3 中断标签

- `[intr]`
- `[ph_sanity]`
- `[ph_stress]`

### 8.4 压力标签

- `[stress]`
- `[sanity]`
- `[ph_base]`
- `[ph1s_stress]`

建议未来项目保留同样的“功能标签 + 强度标签”双层分类。

---

## 9. 各测试文件的功能边界

### 9.1 `base.cc`

覆盖：

- BAR 访问
- ROM 读取/写入
- IPC
- AER 注入
- PBA / slide / config 空间

代表用例：

- `base_exp_rom_r`
- `base_exp_rom_w`
- `ipc`
- `pf0_pba`
- `pf1_pba`

### 9.2 `intr.cc`

覆盖：

- PF / VF 中断触发
- Legacy / MSI / MSI-X 切换
- DMA 中断联调

代表用例：

- `intr_dma_pf`
- `intr_dma_vf_all`
- `intr_pf0_legacy_sanity`
- `intr_pf0_msi_sanity`
- `intr_pf0_msix_sanity`

### 9.3 `mthdma.cc`

覆盖：

- bare DMA 冒烟
- 链式 / 块模式
- MMU/bypass-MMU
- 多通道
- 性能测试
- 错误注入与异常路径
- DMA engine 读写

代表用例：

- `sanity_dma_bare_single_s`
- `sanity_dma_bare_chain_ddr`
- `sanity_dma_bare_block_ddr`
- `perf_dma_bare_single_ddr_mmu`
- `stress_dma_bare_single_ddr`
- `channel_stop`
- `mtdma_engine_rw`
- `sanity_dma_bare_desc_error`

### 9.4 `stress.cc`

覆盖：

- 长时间压力
- 扫描
- 多通道重入
- 24h 型回归

代表用例：

- `stress_mtdma_multi`
- `stress_dma_bare_speed`
- `stress_24h`
- `stress_sanity`

---

## 10. 测试环境的隐含依赖

### 10.1 设备文件依赖

来源：`test/lib/mt_pcie_f.c`

- `/dev/mt_emu_gpu`
- `/dev/mt_emu_dmabuf`
- `/dev/mt_emu_vgpu<N>`

### 10.2 sysfs 元数据依赖

- `/sys/class/misc/<device>/bar0`
- `/sys/class/misc/<device>/bar2`
- `/sys/class/misc/mt_emu_gpu/vf`

### 10.3 外部文件依赖

来源：`test/src/base.cc`

- `../../PHGOP.txt`
  - ROM 比对测试要用

---

## 11. 新项目重建测试集的推荐模板

### 11.1 基础模板

```cpp
TEST_CASE("case_name", "[tag]") {
    uint32_t ch_num = 0;
    uint32_t ch_cnt = 1;
    uint32_t dirs = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM);
    uint32_t desc_direction = DMA_DESC_IN_DEVICE;
    uint32_t desc_cnt = 0;
    uint32_t block_cnt = 0;
    uint64_t sar = 0x0;
    uint64_t dar = 0x0;
    uint64_t size = 512 * 1024;
    int cnt = 1;

    dma_bare_simple_test(ch_num, ch_cnt, dirs, desc_direction,
                         desc_cnt, block_cnt, sar, dar, size, cnt, 0);
}
```

### 11.2 设计原则

1. 每个 `TEST_CASE` 只验证一种参数组合
2. 公共构造逻辑放到 helper
3. 设备访问只通过 `pcief_*` API
4. 断言统一用 `REQUIRE`

---

## 12. 当前仓库的构建兼容性备注

当前仓库在本地工具链下，`test` 编译命中 Catch2 老版本兼容问题：

- `sysconf()` constexpr 报错
- `altStackMem` 不是 integral constant expression

因此未来项目重建测试集时，应把 **Catch2 版本 / 编译器版本 / C++ 标准** 当作环境参数处理，而不是照搬旧依赖状态。

---

## 13. 人工更新检查表

- [ ] `main.cc` 初始化顺序是否变化
- [ ] `test_thr.h` 参数结构是否变化
- [ ] `mt_pcie_f.h` 公开 API 是否变化
- [ ] 设备节点和 sysfs 发现方式是否变化
- [ ] 标签分类是否变化
- [ ] `base` / `intr` / `mthdma` / `stress` 的职责边界是否变化
- [ ] ROM 测试外部依赖是否变化
