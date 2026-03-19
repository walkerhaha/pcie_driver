# DMA 链式传输测试分析

本文只基于仓库里的实际实现来说明 DMA 链式传输测试，**不参考 `examples/docs/`**。  
主要依据：

- `examples/mtdma_baremetal/mtdma_baremetal.h`
- `examples/mtdma_baremetal/mtdma_baremetal.c`
- `examples/mtdma_baremetal/test2_chain_debug.c`

重点分析的是：

- Test 2：`chain mode in single chain`
- Test 3：`chain block mode in single chain`

其中 Test 2 最适合用来说明“DMA 链式传输到底配了什么”。

---

## 1. 先看测试本身在做什么

以 `examples/mtdma_baremetal/mtdma_baremetal.c` 里的 `test_chain_1ch()` 为例：

1. 分配两块主机侧一致性 DMA 缓冲区：
   - `h2d_buf`：Host → Device
   - `d2h_buf`：Device → Host
2. 给 `h2d_buf` 填测试数据。
3. 对 **RD 通道 0** 提交一次链式 H2D 传输：
   - 源地址 `sar = h2d_bus`
   - 目的地址 `dar = dev_addr`
4. 轮询等待完成。
5. 对 **WR 通道 0** 提交一次链式 D2H 传输：
   - 源地址 `sar = dev_addr`
   - 目的地址 `dar = d2h_bus`
6. 再轮询等待完成。
7. `memcmp(h2d_buf, d2h_buf)` 校验往返数据是否一致。

也就是说，链式测试本质上不是“只测 descriptor 能不能串起来”，而是：

> 先用一串 descriptor 把主机数据搬到设备 DDR，  
> 再用另一串 descriptor 把设备 DDR 数据搬回主机，  
> 最后用 `memcmp` 验证整条链路正确。

---

## 2. 配置寄存器分两层：公共寄存器 + 通道寄存器

### 2.1 公共寄存器：模块初始化时先配一次

这部分在 `mtdma_comm_init()` 里完成。

| 寄存器 | 偏移 | 作用 | 示例值 |
|---|---:|---|---:|
| `REG_COMM_CH_NUM` | `0x400` | 告知硬件总通道数减 1 | `MTDMA_NUM_DMA_CH - 1`，即 `63` |
| `REG_COMM_MST0_BLEN` | `0x408` | 数据路径突发长度 | `MTDMA_BLEN_VAL` |
| `REG_COMM_MST1_BLEN` | `0x608` | 描述符 fetch 路径突发长度 | `MTDMA_BLEN_VAL` |
| `REG_COMM_ALARM_IMSK` | `0xC00` | 全局告警中断屏蔽 | `0xffffffff` |
| `REG_COMM_RD_IMSK_C32` | `0xC20` | RD 通道 0~31 聚合中断屏蔽 | `0` |
| `REG_COMM_WR_IMSK_C32` | `0xC40` | WR 通道 0~31 聚合中断屏蔽 | `0` |
| `REG_COMM_WORK_STS` | `0xD00` | 查看 DMA 是否空闲 | 只读检查 |

这里最容易误解的一点是 `REG_COMM_RD/WR_IMSK_C32 = 0`。

在这个 baremetal 示例里，完成检测用的是**轮询通道本地状态**，不是走 MSI 中断，所以这里没有打开聚合中断。  
这并不影响链式传输本身，影响的只是“完成后怎么知道它结束了”。

---

### 2.2 通道寄存器：每发起一次链式传输都要配

这部分在 `mtdma_submit_chain()` 里完成，顺序非常关键。

对一次链式传输，实际写寄存器的顺序是：

1. `REG_CH_LBAR_BASIC`
2. `REG_CH_INTR_IMSK`
3. 第 0 号描述符寄存器区（`0x400 ~ 0x41c`）
4. BAR2 里的后续描述符链表区
5. `REG_CH_DIRECTION`
6. `REG_CH_ENABLE = dir_flags`
7. 回读 `REG_CH_ENABLE` 做 flush
8. `REG_CH_ENABLE = dir_flags | CH_EN_ENABLE`，正式 doorbell

对应寄存器如下。

| 寄存器 | 偏移 | 什么时候写 | 作用 |
|---|---:|---|---|
| `REG_CH_LBAR_BASIC` | `0x0D4` | 第一步 | 配置链式模式和后续描述符个数 |
| `REG_CH_INTR_IMSK` | `0x0C4` | 第二步 | 通道内中断屏蔽，示例里写 `0` |
| `REG_CH_DESC_OPT` | `0x400` | 第三步 | desc0 的 `desc_op` |
| `REG_CH_ACNT` | `0x404` | 第三步 | desc0 的 `cnt=size-1` |
| `REG_CH_SAR_L/H` | `0x408/0x40C` | 第三步 | desc0 源地址 |
| `REG_CH_DAR_L/H` | `0x410/0x414` | 第三步 | desc0 目的地址 |
| `REG_CH_LAR_L/H` | `0x418/0x41C` | 第三步 | desc0 下一描述符地址 |
| `REG_CH_DIRECTION` | `0x004` | 第五步 | 方向相关标志 |
| `REG_CH_ENABLE` | `0x000` | 第六、八步 | 先预写，再置 `CH_EN_ENABLE` 启动 |
| `REG_CH_INTR_RAW` | `0x0C8` | 完成后轮询 | 查看 `DONE` / 错误位 |
| `REG_CH_STATUS` | `0x0D0` | 调试时查看 | 通道忙闲状态 |

---

## 3. `REG_CH_LBAR_BASIC` 怎么配

`REG_CH_LBAR_BASIC` 的格式在示例里写得很明确：

- `[31:16]`：**第 1 号到最后一个**描述符的数量
- `[0]`：`chain_en`

所以：

```c
LBAR_BASIC = (extra_descs << 16) | 1;
```

如果总共要跑 4 个描述符：

- `desc0` 在寄存器区
- `desc1 ~ desc3` 在链表区

那么：

- `extra_descs = 3`
- `LBAR_BASIC = (3 << 16) | 1 = 0x00030001`

这也是 Test 2 / Test 3 的默认配置，因为：

```c
#define MTDMA_CHAIN_DESC_NUM 4
```

---

## 4. 描述符格式是什么样

描述符结构体定义在 `examples/mtdma_baremetal/mtdma_baremetal.h`：

```c
struct mtdma_desc {
	u32 desc_op;
	u32 cnt;
	u32 sar_lo;
	u32 sar_hi;
	u32 dar_lo;
	u32 dar_hi;
	u32 lar_lo;
	u32 lar_hi;
} __packed;
```

总大小固定 **32 字节**。

### 4.1 每个字段含义

| 字段 | 含义 |
|---|---|
| `desc_op` | 控制位，`BIT(0)=INTR_EN`，`BIT(1)=CHAIN_EN` |
| `cnt` | 传输字节数减 1，不是原始字节数 |
| `sar_lo/hi` | 源地址 64 位 |
| `dar_lo/hi` | 目的地址 64 位 |
| `lar_lo/hi` | 下一描述符物理地址 64 位 |

### 4.2 `desc_op` 的用法

中间描述符：

```c
desc_op = DESC_CHAIN_EN;
```

最后一个描述符：

```c
desc_op = DESC_INTR_EN;
```

也就是：

- **中间项**：告诉硬件“继续按 `LAR` 去抓下一个 descriptor”
- **最后一项**：告诉硬件“这条链结束了，并置完成状态”

最后一个描述符的 `lar` 必须写 `0`。

---

## 5. desc0 和 desc1..N 放在哪里

这是链式模式里最关键的一点。

### 5.1 desc0 不在 BAR2，而是在通道寄存器区

第 0 号描述符不是放在链表内存里，而是直接映射成通道寄存器：

```text
rg_base + 0x400
```

也就是：

- `desc_op` → `REG_CH_DESC_OPT`
- `cnt`     → `REG_CH_ACNT`
- `sar`     → `REG_CH_SAR_L/H`
- `dar`     → `REG_CH_DAR_L/H`
- `lar`     → `REG_CH_LAR_L/H`

硬件是从这个“固定入口”开始抓第一项 descriptor 的。

### 5.2 desc1..N 放在 BAR2 描述符链表区

BAR2 里专门留了一段描述符区：

```c
#define MTDMA_DESC_LIST_BASE 0x70000000UL
#define MTDMA_LL_CH_STRIDE   0x10000UL
```

某个通道的链表区虚拟地址：

```c
ll_vaddr = bar2 + MTDMA_DESC_LIST_BASE
         + (2 * ch_idx + is_wr) * MTDMA_LL_CH_STRIDE;
```

对应的设备侧物理地址：

```c
ll_ddr = MTDMA_BAR2_DEVICE_BASE + MTDMA_DESC_LIST_BASE
       + (2 * ch_idx + is_wr) * MTDMA_LL_CH_STRIDE;
```

以 `ch0` 为例：

| 通道 | `ll_vaddr` BAR2 偏移 | `ll_ddr` 设备地址 |
|---|---:|---:|
| `rd_ch[0]` | `0x70000000` | `0x010070000000` |
| `wr_ch[0]` | `0x70010000` | `0x010070010000` |

所以对于 `rd_ch[0]`：

- `desc1` 在设备地址 `0x010070000000`
- `desc2` 在设备地址 `0x010070000020`
- `desc3` 在设备地址 `0x010070000040`

因为每个描述符 32B，也就是 `0x20`。

---

## 6. 链式测试的完整提交流程

下面按 Test 2 的真实逻辑，把链式传输线性展开。

### 6.1 H2D 链式传输

调用：

```c
mtdma_submit_chain(&mdev->rd_ch[0], rd_ll, rd_ddr,
		   (u64)h2d_bus, dev_addr,
		   MTDMA_XFER_SIZE, extra, 0);
```

这里参数的意思是：

- 通道：`rd_ch[0]`
- 链表写入位置：`rd_ll`
- 链表设备地址：`rd_ddr`
- 源地址：主机 DMA 地址 `h2d_bus`
- 目的地址：设备 DDR 地址 `dev_addr`
- 总传输大小：`64 KiB`
- 后续描述符数：`extra = 3`
- 方向标志：`dir_flags = 0`

执行过程是：

1. `REG_CH_LBAR_BASIC = 0x00030001`
2. `REG_CH_INTR_IMSK = 0`
3. 构造 desc0，写到 `rg_base + 0x400`
4. 构造 desc1~desc3，写到 `rd_ll`
5. `REG_CH_DIRECTION = 0`
6. `REG_CH_ENABLE = 0`
7. 回读 `REG_CH_ENABLE`
8. `REG_CH_ENABLE = CH_EN_ENABLE`

### 6.2 D2H 链式传输

调用：

```c
mtdma_submit_chain(&mdev->wr_ch[0], wr_ll, wr_ddr,
		   dev_addr, (u64)d2h_bus,
		   MTDMA_XFER_SIZE, extra, CH_EN_DUMMY);
```

和 H2D 的区别只有两点：

1. 源/目的地址反过来：
   - `sar = dev_addr`
   - `dar = d2h_bus`
2. `dir_flags = CH_EN_DUMMY`

这里 `CH_EN_DUMMY` 很关键，示例把它用于 D2H，目的是让主机侧更可靠地看到设备写回的数据。

---

## 7. 一个完整示例：4 个描述符搬 64 KiB

示例里的默认参数：

```c
MTDMA_XFER_SIZE      = 64 * 1024 = 0x10000
MTDMA_CHAIN_DESC_NUM = 4
extra_descs          = 3
each                 = total_size / 4 = 0x4000
last_bytes           = 0x10000 - 0x4000 * 3 = 0x4000
```

假设：

- `h2d_bus = 0x0000001234000000`
- `dev_addr = 0x010000100000`
- `rd_ll_ddr = 0x010070000000`

那么 H2D 的 4 个描述符会被展开成下面这样。

### 7.1 descriptor 链内容

| desc | 存放位置 | `desc_op` | `cnt` | `sar` | `dar` | `lar` |
|---|---|---|---:|---:|---:|---:|
| 0 | `rg_base + 0x400` | `CHAIN_EN` | `0x3fff` | `0x0000001234000000` | `0x010000100000` | `0x010070000000` |
| 1 | `rd_ll + 0x00` | `CHAIN_EN` | `0x3fff` | `0x0000001234004000` | `0x010000104000` | `0x010070000020` |
| 2 | `rd_ll + 0x20` | `CHAIN_EN` | `0x3fff` | `0x0000001234008000` | `0x010000108000` | `0x010070000040` |
| 3 | `rd_ll + 0x40` | `INTR_EN`  | `0x3fff` | `0x000000123400c000` | `0x01000010c000` | `0` |

注意两个点：

1. `cnt = bytes - 1`，所以 16 KiB 要写成 `0x4000 - 1 = 0x3fff`
2. `lar` 指向的是**下一项描述符的设备物理地址**，不是 BAR2 虚拟地址

### 7.2 这个链会怎么跑

硬件执行顺序就是：

```text
先抓 desc0
  -> 搬 0x4000 字节
  -> 看到 CHAIN_EN=1，跳到 LAR=0x010070000000

再抓 desc1
  -> 再搬 0x4000 字节
  -> 跳到 0x010070000020

再抓 desc2
  -> 再搬 0x4000 字节
  -> 跳到 0x010070000040

最后抓 desc3
  -> 再搬 0x4000 字节
  -> 看到 INTR_EN=1，置 DONE，不再跳转
```

这样 4 段一共正好是 `4 * 16 KiB = 64 KiB`。

---

## 8. Test 3 和 Test 2 的区别

Test 3 不是描述符格式变了，而是**把 Test 2 这套链式提交，连续做了多次 block**。

默认配置：

```c
MTDMA_BLOCK_CNT = 4
```

于是 Test 3 会循环 4 次：

1. 第 0 块：对 `dev_addr = ch_dev_addr(0, 0)` 做一轮 H2D + D2H
2. 第 1 块：对 `dev_addr = ch_dev_addr(0, 1)` 再做一轮
3. 第 2 块：继续
4. 第 3 块：继续

所以 Test 3 本质上是：

> “同一种 4-descriptor 链式搬运”，  
> 但把目标设备地址和主机 buffer 偏移不断往后挪，  
> 连续做 4 个 block。

寄存器配置方式和描述符格式都**没有变化**。

---

## 9. 实际排障时最值得盯的几个点

如果链式测试不过，通常优先看下面几个地方：

1. **`LBAR_BASIC` 是否正确**
   - 4 个 descriptor 必须是 `0x00030001`
2. **desc0 是否真的写在 `rg_base + 0x400`**
   - 很多人会误以为 desc0 也在 BAR2
3. **`LAR` 是否写了设备物理地址**
   - 不能写 CPU 虚拟地址
4. **`cnt` 是否是 `size - 1`**
   - 写成 `size` 会出错
5. **最后一个 descriptor 是否是 `INTR_EN` 且 `LAR=0`**
6. **D2H 时是否带了 `CH_EN_DUMMY`**
7. **完成检测是否看的是 `REG_CH_INTR_RAW`**
   - 这个示例不是靠 MSI，而是靠轮询

如果想看更详细的运行时展开，可以直接看：

- `examples/mtdma_baremetal/test2_chain_debug.c`

这个调试模块会把每个 descriptor 的：

- 计划值
- 写入位置
- `LAR` 指向
- 回读值

都打印出来，最适合对照硬件现象。
