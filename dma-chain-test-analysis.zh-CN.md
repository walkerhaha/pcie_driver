# DMA 链式传输测试提炼（仅基于 `driver/` 与 `test/`）

本文**忽略 `examples/` 和 `docs/` 两个目录**，只基于以下实现提炼 DMA 链式传输测试：

- `driver/mt-emu-mtdma-bare.h`
- `driver/mt-emu-mtdma-bare.c`
- `driver/mt-emu-drv.h`
- `driver/mt-emu-ioctl.c`
- `test/lib/mt_pcie_f.c`
- `test/src/mthdma.cc`
- `BARE_DMA_HOWTO.md`

如果只想抓重点，可以先记住这一句话：

> 用户态测试通过 `MT_IOCTL_MTDMA_BARE_RW` 把 `desc_cnt / block_cnt / sar / dar / size` 送进内核，  
> 内核在 `dma_bare_xfer()` 里把第 0 个描述符写到通道寄存器区，把后续描述符写到链表区，  
> 然后写 `REG_DMA_CH_ENABLE` 启动 DMA，并等待中断完成。

---

## 1. 链式传输测试入口在哪里

### 1.1 用户态测试入口

链式测试在 `test/src/mthdma.cc` 里最直接的两个用例是：

- `sanity_dma_bare_chain_ddr`
- `sanity_dma_bare_block_ddr`

其中：

### 链式测试 `sanity_dma_bare_chain_ddr`

```c
uint32_t des_cnt              = 32;
uint32_t test_desc_direction  = DMA_DESC_IN_DEVICE;
uint32_t test_desc_cnt        = des_cnt - 1;
uint32_t test_block_cnt       = 0;
uint64_t test_size            = des_cnt * 1024 * 4;
```

含义是：

- 总描述符数 = `32`
- 传给驱动的 `desc_cnt = 31`
- `block_cnt = 0`，所以这是真正的“单链式模式”
- 总大小 = `32 * 4KB = 128KB`

### 块模式测试 `sanity_dma_bare_block_ddr`

```c
uint32_t test_desc_cnt  = 8;
uint32_t test_block_cnt = 32;
uint64_t test_size      = test_block_cnt * 1024;
```

它不是改了描述符格式，而是：

- 每个 block 用一组链式描述符
- 共跑 `32` 个 block

所以：

- `chain_ddr`：单条链
- `block_ddr`：多条链按 block 串起来

---

## 2. 用户态参数是怎么传进内核的

用户态封装函数在 `test/lib/mt_pcie_f.c`：

```c
int pcief_dma_bare_xfer(uint32_t data_direction,
			uint32_t desc_direction,
			uint32_t desc_cnt,
			uint32_t block_cnt,
			uint32_t ch_num,
			uint64_t sar,
			uint64_t dar,
			uint32_t size,
			uint32_t timeout_ms)
```

它会构造 `struct dma_bare_rw`，然后发这个 ioctl：

```c
ioctl(fd, MT_IOCTL_MTDMA_BARE_RW, emu_param);
```

而 `struct dma_bare_rw` 定义在 `driver/mt-emu-drv.h`：

```c
struct dma_bare_rw {
	unsigned long long sar;
	unsigned long long dar;
	unsigned int data_direction;
	unsigned int desc_direction;
	unsigned int desc_cnt;
	unsigned int block_cnt;
	unsigned int size;
	unsigned int ch_num;
	unsigned int timeout_ms;
};
```

这里和链式测试最相关的字段是：

| 字段 | 含义 |
|---|---|
| `sar` | 源地址 |
| `dar` | 目的地址 |
| `data_direction` | H2H / H2D / D2H / D2D |
| `desc_direction` | 描述符在设备侧还是主机侧 |
| `desc_cnt` | **后续描述符数量**；总描述符数 = `desc_cnt + 1` |
| `block_cnt` | 块模式的块数；`0` 表示普通链式 |
| `size` | 总字节数 |
| `ch_num` | 通道号 |

---

## 3. ioctl 进入内核后的执行路径

内核入口在 `driver/mt-emu-ioctl.c`：

```text
MT_IOCTL_MTDMA_BARE_RW
  -> 取出 struct dma_bare_rw
  -> 根据 data_direction 选择 rd_ch / wr_ch
  -> 调 dma_bare_xfer(...)
```

选择通道的规则是：

- `DMA_MEM_TO_MEM`、`DMA_MEM_TO_DEV`  
  走 `rd_ch[]`
- `DMA_DEV_TO_MEM`、`DMA_DEV_TO_DEV`  
  走 `wr_ch[]`

这点很重要，因为：

> 用户态看到的是“数据方向”，  
> 驱动内部要先根据方向选中对应 RD/WR 通道，  
> 后面的寄存器写入都是在这个 `bare_ch->info.rg_vaddr` 上完成的。

---

## 4. DMA 链式传输到底要配置哪些寄存器

### 4.1 公共寄存器：模块初始化时先配

这部分在 `driver/mt-emu-mtdma-bare.c` 的 `mtdma_comm_init()`。

| 寄存器 | 定义位置 | 作用 |
|---|---|---|
| `REG_DMA_COMM_BASIC_PARAM` | `mt-emu-mtdma-bare.h` | 读版本号 |
| `REG_DMA_COMM_COMM_ALARM_IMSK` | `mt-emu-mtdma-bare.h` | 告警中断屏蔽 |
| `REG_DMA_COMM_CH_NUM` | `mt-emu-mtdma-bare.h` | 配置 DMA 总通道数减 1 |
| `REG_DMA_COMM_MST0_BLEN` | `mt-emu-mtdma-bare.h` | 数据通路 burst 长度 |
| `REG_DMA_COMM_MST1_BLEN` | `mt-emu-mtdma-bare.h` | 描述符通路 burst 长度 |
| `REG_DMA_COMM_RD_MRG_PF0_IMSK_C32/C64` | `mt-emu-mtdma-bare.h` | RD 聚合中断掩码 |
| `REG_DMA_COMM_WR_MRG_PF0_IMSK_C32/C64` | `mt-emu-mtdma-bare.h` | WR 聚合中断掩码 |
| `REG_DMA_COMM_WORK_STS` | `mt-emu-mtdma-bare.h` | 查看 DMA 是否忙 |

从代码上看，初始化逻辑是：

1. 读版本号
2. 写告警屏蔽
3. 写 `REG_DMA_COMM_CH_NUM = PCIE_DMA_CH_NUM - 1`
4. 写 `REG_DMA_COMM_MST0_BLEN`
5. 写 `REG_DMA_COMM_MST1_BLEN`
6. 给 VF/通道相关寄存器做 OSID 和中断 mask 初始化

这部分不是“每次链式传输都写”，而是 DMA 子系统初始化时写一次。

---

### 4.2 通道寄存器：每次链式传输都会写

真正和链式测试直接相关的是这些通道寄存器：

| 寄存器 | 偏移 | 作用 |
|---|---:|---|
| `REG_DMA_CH_ENABLE` | `0x000` | 启动 DMA |
| `REG_DMA_CH_DIRECTION` | `0x004` | 方向标志 |
| `REG_DMA_CH_MMU_ADDR_TYPE` | `0x010` | MMU 地址类型（仅某些配置下使用） |
| `REG_DMA_CH_INTR_IMSK` | `0x0C4` | 通道中断屏蔽 |
| `REG_DMA_CH_INTR_RAW` | `0x0C8` | 中断原始状态 |
| `REG_DMA_CH_STATUS` | `0x0D0` | 通道忙状态 |
| `REG_DMA_CH_LBAR_BASIC` | `0x0D4` | 链表基础配置 |
| `REG_DMA_CH_DESC_OPT` | `0x400` | 第 0 个描述符的 `desc_op` |
| `REG_DMA_CH_ACNT` | `0x404` | 第 0 个描述符的 `cnt` |
| `REG_DMA_CH_SAR_L/H` | `0x408/0x40C` | 第 0 个描述符的源地址 |
| `REG_DMA_CH_DAR_L/H` | `0x410/0x414` | 第 0 个描述符的目的地址 |
| `REG_DMA_CH_LAR_L/H` | `0x418/0x41C` | 第 0 个描述符的下一项地址 |

链式模式里，`dma_bare_xfer()` 的关键寄存器顺序可以提炼成：

1. 根据方向计算 `direction`
2. 计算 `chain_en = (desc_cnt == 0) ? 0 : 1`
3. 先把 descriptor 写好
4. 写 `REG_DMA_CH_LBAR_BASIC`
5. 写 `REG_DMA_CH_INTR_IMSK = 0`
6. 写 `REG_DMA_CH_DIRECTION = direction`
7. 写 `REG_DMA_CH_ENABLE = DMA_CH_EN_BIT_ENABLE`
8. 等待中断完成

---

## 5. `REG_DMA_CH_LBAR_BASIC` 怎么理解

驱动里它是这么写的：

```c
u32 ch_lbar_basic = (desc_cnt_tmp << 16) | chain_en;
SET_CH_32(bare_ch, REG_DMA_CH_LBAR_BASIC, ch_lbar_basic);
```

所以可以直接提炼成：

- `[31:16] = 后续描述符个数`
- `[0] = chain_en`

也就是说：

- 单描述符模式：`desc_cnt = 0`
  - `chain_en = 0`
  - `LBAR_BASIC = 0`
- 链式模式：总描述符数 = `N`
  - `desc_cnt = N - 1`
  - `chain_en = 1`
  - `LBAR_BASIC = ((N - 1) << 16) | 1`

### 例子

测试 `sanity_dma_bare_chain_ddr`：

- `des_cnt = 32`
- `test_desc_cnt = 31`

所以：

```text
LBAR_BASIC = (31 << 16) | 1 = 0x001f0001
```

---

## 6. 描述符格式是什么

描述符结构在 `driver/mt-emu-mtdma-bare.h`：

```c
struct dma_ch_desc {
	uint32_t desc_op;
	uint32_t cnt;
	union { uint64_t reg; struct { uint32_t lsb; uint32_t msb; }; } sar;
	union { uint64_t reg; struct { uint32_t lsb; uint32_t msb; }; } dar;
	union { uint64_t reg; struct { uint32_t lsb; uint32_t msb; }; } lar;
} __packed;
```

大小固定为：

```text
4 + 4 + 8 + 8 + 8 = 32 字节
```

字段含义：

| 字段 | 含义 |
|---|---|
| `desc_op` | 描述符控制字 |
| `cnt` | 本描述符搬运字节数减 1 |
| `sar` | 源地址 64 位 |
| `dar` | 目的地址 64 位 |
| `lar` | 下一描述符地址 64 位 |

与之相关的位定义也在同一个头文件里：

```c
#define DMA_CH_DESC_BIT_INTR_EN  BIT(0)
#define DMA_CH_DESC_BIT_CHAIN_EN BIT(1)
```

不过要注意，当前 `driver/mt-emu-mtdma-bare.c` 在链式分支里直接把 `desc_op` 写成数值 `0` 或 `(desc_cnt_tmp * 65536 + 1)`，没有像示例代码那样显式写 `CHAIN_EN` / `INTR_EN` 宏名。  
从实现意图上看，它仍然是在编码“是否还有后续描述符 / 是否结束并触发完成”，只是表达方式更底层。

---

## 7. 第 0 个描述符和后续描述符分别放哪

这是链式模式最关键的一点。

### 7.1 第 0 个描述符在寄存器区

在 `dma_bare_xfer()` 里：

```c
if (i == 0)
    lli = bare_ch->info.rg_vaddr + REG_DMA_CH_DESC_OPT;
```

也就是说：

- desc0 不在链表内存
- desc0 直接写在通道寄存器窗口的 `0x400` 开始位置

对应关系就是：

| 字段 | 寄存器 |
|---|---|
| `desc_op` | `REG_DMA_CH_DESC_OPT` |
| `cnt` | `REG_DMA_CH_ACNT` |
| `sar` | `REG_DMA_CH_SAR_L/H` |
| `dar` | `REG_DMA_CH_DAR_L/H` |
| `lar` | `REG_DMA_CH_LAR_L/H` |

### 7.2 desc1 ~ descN 在链表区

如果 `desc_direction == DMA_DESC_IN_DEVICE`：

```c
lli = &(((struct dma_ch_desc *)bare_ch->info.ll_vaddr)[i - 1]);
lar = bare_ch->info.ll_laddr + (i * sizeof(struct dma_ch_desc));
```

如果 `desc_direction == DMA_DESC_IN_HOST`：

```c
lli = (struct dma_ch_desc *)(bare_ch->info.ll_vaddr_system + (i - 1) * sizeof(struct dma_ch_desc));
lar = bare_ch->info.ll_laddr_system + i * sizeof(struct dma_ch_desc);
```

所以可以提炼成：

- desc0：固定在寄存器区
- desc1 开始：放在 `ll_vaddr` / `ll_vaddr_system`
- `lar` 指向的是**下一项描述符地址**

---

## 8. 链式测试怎么把 128KB 切成 32 个描述符

`sanity_dma_bare_chain_ddr` 的参数是：

- 总大小：`128KB`
- 总描述符数：`32`
- `desc_cnt = 31`
- `block_cnt = 0`

驱动在普通链式模式里这样切分：

```c
elm_cnt = size_tmp / (desc_cnt_tmp + 1);
```

所以这里：

```text
elm_cnt = 128KB / 32 = 4KB
```

然后每个描述符写：

- `cnt = 4KB - 1 = 0xFFF`
- `sar += 4KB`
- `dar += 4KB`
- `lar = 下一项描述符地址`

### 一个线性的 4 项示意

如果把 32 项简化成 4 项来看，逻辑是一样的：

| desc | 放置位置 | `cnt` | `sar` | `dar` | `lar` |
|---|---|---:|---:|---:|---:|
| 0 | `rg_vaddr + 0x400` | `seg_size - 1` | `sar + 0*seg` | `dar + 0*seg` | `next desc` |
| 1 | `ll_vaddr + 0x00` | `seg_size - 1` | `sar + 1*seg` | `dar + 1*seg` | `next desc` |
| 2 | `ll_vaddr + 0x20` | `seg_size - 1` | `sar + 2*seg` | `dar + 2*seg` | `next desc` |
| 3 | `ll_vaddr + 0x40` | `last - 1` | `sar + 3*seg` | `dar + 3*seg` | 链尾 |

这里只是帮助理解，仓库真实测试用的是 32 项。

---

## 9. 块模式和普通链式模式有什么不同

在 `dma_bare_xfer()` 里：

- `block_cnt == 0`：普通链式分支
- `block_cnt != 0`：块模式分支

块模式的差别不是 descriptor 结构变了，而是：

1. 外层多了一层 `for (j = 0; j < block_cnt; j++)`
2. 每个 block 里继续生成一串 descriptor
3. 最后一个 descriptor 的 `desc_op` 会额外编码下一个 block 的信息

因此可以这么理解：

- 普通链式：一条链完成一次总传输
- block 模式：多条链首尾衔接，连续完成多块传输

---

## 10. 真正排障时优先看什么

如果链式测试不过，优先核对这几个点：

1. **`desc_cnt` 的语义有没有搞错**
   - 总描述符数 = `desc_cnt + 1`
2. **`REG_DMA_CH_LBAR_BASIC` 是否正确**
   - 应该是 `(desc_cnt << 16) | chain_en`
3. **第 0 个 descriptor 是否确实写在 `rg_vaddr + 0x400`**
4. **后续 descriptor 是否按 32B 间隔排布**
5. **`cnt` 是否写成 `字节数 - 1`**
6. **`sar` / `dar` 是否按每段长度递增**
7. **`desc_direction` 是否匹配**
   - `DMA_DESC_IN_DEVICE`：链表写到设备侧链表区
   - `DMA_DESC_IN_HOST`：链表在系统内存
8. **启动顺序是否正确**
   - 先写 descriptor，再写 `REG_DMA_CH_ENABLE`
9. **是否真的等到了完成中断**
   - 驱动当前是 `wait_for_completion_timeout()`
   - ISR 在 `dma_bare_isr()` 里清 `REG_DMA_CH_INTR_RAW` 并 `complete()`

---

## 11. 最短总结

如果只保留最核心的结论，可以压缩成下面 6 条：

1. 链式测试入口在  
   `test/src/mthdma.cc`
2. 用户态通过  
   `test/lib/mt_pcie_f.c` 的 `pcief_dma_bare_xfer()`  
   发送 `MT_IOCTL_MTDMA_BARE_RW`
3. 内核在  
   `driver/mt-emu-ioctl.c`  
   里转到 `dma_bare_xfer()`
4. 描述符格式在  
   `driver/mt-emu-mtdma-bare.h`  
   是固定 **32B**
5. desc0 写到通道寄存器区 `0x400` 起始位置，后续 desc 写到链表区
6. 链式模式核心寄存器就是：
   - `REG_DMA_CH_LBAR_BASIC`
   - `REG_DMA_CH_INTR_IMSK`
   - `REG_DMA_CH_DIRECTION`
   - `REG_DMA_CH_ENABLE`
   - 以及 desc0 对应的 `REG_DMA_CH_DESC_OPT ~ REG_DMA_CH_LAR_H`
