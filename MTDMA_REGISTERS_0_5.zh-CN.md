# MTDMA 寄存器整理（0-5章）

> 说明：
> - 本文档将现有提炼结果整理为适合继续修改和粘贴的纯 Markdown 版本。
> - 分类按 `Common / RD / WR / Interrupt / MMU` 输出。
> - 内容基于当前仓库中的旧驱动实现、寄存器头文件与寄存器 dump 脚本整理。
> - 个别寄存器在旧代码中存在命名冲突，文中已单独标注。

## 0. 地址约定

### 0.1 DMA 总体地址分区

| 区域 | 基址 | 说明 |
|---|---:|---|
| DMA Common | 每个项目基址都可能变化，因此common base addr应该设成全局可配的 | 全局配置、通道汇总状态、merge interrupt、OSID/Context 等 |
| DMA Channel | common_base_addr+0x3000 | 每个 channel 的寄存器窗口 |

### 0.2 Channel 地址布局

每个 channel 步长为 `0x1000`：

- `RD chN base = common_base_addr + N * 0x1000`
- `WR chN base = channel_base_addr + N * 0x1000 + 0x800`

---

## 1. Common 寄存器表

> 基址：common_base_addr

### 1.1 基础配置类

| 名称 | 偏移 | 含义 | 当前驱动中的用法/值 |
|---|---:|---|---|
| `REG_DMA_COMM_BASIC_PARAM` | `0x000` | DMA 基本参数/版本 | 只读打印版本号 |
| `REG_DMA_COMM_COMM_ENABLE` | `0x010` | Common 功能总开关 | 代码注释提示 `BIT(0)` 可能是 `osid_en`，但当前未开启 |
| `REG_DMA_COMM_OSID_SUPER` | `0x014` | OSID super 配置 | 已定义，当前代码未实际写入 |
| `REG_DMA_COMM_CH_OSID(i)` | `0x020 + 4 * i` | 头文件命名为通道 OSID | 当前初始化时写 `0x1`；但 dump 脚本把这组寄存器打印为 `dma_ch_secure`。现阶段更建议把它视为“OSID/secure 属性寄存器”，不要直接当成最终 OSID 值表 |
| `0x018` | `dma_ccmode_disable`（仅脚本命名） | 可能是 cache-coherent / cc mode 相关控制 | 仅在 dump 脚本中打印，正式头文件未定义 |
| `REG_DMA_COMM_CH_NUM` | `0x400` | 通道数量配置 | 写 `PCIE_DMA_CH_NUM - 1`，当前即 63 |
| `REG_DMA_COMM_MST0_BLEN` | `0x408` | MST0 burst 长度 | 当前写 `(4<<4) \| 4 = 0x44` |
| `REG_DMA_COMM_MST0_CACHE` | `0x420` | MST0 cache 属性 | dump 脚本会读取，当前驱动未写入配置 |
| `REG_DMA_COMM_MST0_PROT` | `0x424` | MST0 prot 属性 | dump 脚本会读取，当前驱动未写入配置 |
| `REG_DMA_COMM_MST2_CACHE` | `0x428` | MST2 cache 属性 | 只定义 / 脚本打印 |
| `REG_DMA_COMM_MST1_BLEN` | `0x608` | MST1 burst 长度 | 当前写 `0x44` |
| `REG_DMA_COMM_MST1_CACHE` | `0x620` | MST1 cache 属性 | dump 脚本会读取，当前驱动未写入配置 |
| `REG_DMA_COMM_MST1_PROT` | `0x624` | MST1 prot 属性 | dump 脚本会读取，当前驱动未写入配置 |
| `0x628` | `dma_mst3_cache`（脚本命名） | 可能是另一路 cache 属性 | 仅脚本打印 |

### 1.2 Common 状态 / 汇总类

| 名称 | 偏移 | 含义 | 说明 |
|---|---:|---|---|
| `REG_DMA_COMM_WORK_STS` | `0xD00` | DMA 工作状态 | 初始化后会检查是否 busy |
| `0x1000 + 4 * ch` | `dma_rch_fc`（脚本命名） | RD channel flow control 参数 | 仅脚本打印 |
| `0x1800 + 4 * ch` | `dma_wch_fc`（脚本命名） | WR channel flow control 参数 | 仅脚本打印 |
| `0x2000 + 4 * ch` | `dma_ch_osid`（脚本命名） | 更像真正的 per-channel OSID 值表 | 当前代码按通道索引写入对应编号 |
| `0x2800 + 4 * ch` | `dma_ch_context_id`（脚本命名） | 每通道 context id | 仅脚本打印 |

---

## 2. RD Channel 寄存器表

> RD chN 基址：`0x383000 + N * 0x1000`

| 名称 | 相对 RD 基址偏移 | 含义 | 当前驱动中的 value / 语义 |
|---|---:|---|---|
| `REG_DMA_CH_ENABLE` | `0x000` | 通道使能 | `BIT(0)` 启动 |
| `REG_DMA_CH_DIRECTION` | `0x004` | 方向 / 模式控制 | 见后文“方向编码” |
| `REG_DUMMY_CH_ADDR_L` | `0x008` | dummy read 地址低 32 位 | 某些模式下读取 |
| `REG_DUMMY_CH_ADDR_H` | `0x00C` | dummy read 地址高 32 位 | 同上 |
| `REG_DMA_CH_MMU_ADDR_TYPE` | `0x010` | MMU 地址类型 | `0x0 / 0x1 / 0x100 / 0x101` |
| `REG_DMA_CH_FC` | `0x020` | flow control | core 路径会写 `BIT(0) \| (WCH_FC_THLD<<1)` |
| `REG_DMA_CH_USER` | `0x024` | 用户属性 | 已定义，当前未实际使用 |
| `0x050` | `dma_rch_pagefault_raw` | RD pagefault raw | 脚本打印 |
| `0x054` | `dma_rch_pagefault_imsk` | RD pagefault mask | 脚本打印 |
| `0x058` | `dma_rch_pagefault_sts` | RD pagefault status | 脚本打印 |
| `0x05C` | `dma_rch_pagefault_vpage_h` | RD fault vpage 高位 | 脚本打印 |
| `0x060` | `dma_rch_pagefault_vpage_l` | RD fault vpage 低位 | 脚本打印 |
| `REG_DMA_CH_INTR_IMSK` | `0x0C4` | 通道中断 mask | 当前多处直接写 `0` |
| `REG_DMA_CH_INTR_RAW` | `0x0C8` | 通道中断 raw / 写回清中断 | ISR 主要读写它 |
| `REG_DMA_CH_INTR_STATUS` | `0x0CC` | 通道中断状态 | PF / VF ISR 用来先判定 done |
| `REG_DMA_CH_STATUS` | `0x0D0` | 通道状态 | `BIT(0)` = busy |
| `REG_DMA_CH_LBAR_BASIC` | `0x0D4` | 链表基本参数 | `bits[31:16]=desc 数`, `bit0=chain_en` |
| `REG_DMA_CH_DESC_OPT` | `0x400` | 第 0 个描述符起始地址 | desc0 放在寄存器区 |
| `REG_DMA_CH_ACNT` | `0x404` | 计数字段 | 对应 desc 的 `cnt` |
| `REG_DMA_CH_SAR_L/H` | `0x408 / 0x40C` | 源地址 | desc 字段 |
| `REG_DMA_CH_DAR_L/H` | `0x410 / 0x414` | 目的地址 | desc 字段 |
| `REG_DMA_CH_LAR_L/H` | `0x418 / 0x41C` | 下一个描述符地址 | desc 字段 |

---

## 3. WR Channel 寄存器表

> WR chN 基址：`0x383000 + N * 0x1000 + 0x800`

| 名称 | 相对 WR 基址偏移 | 绝对窗口偏移示例 | 含义 | 当前驱动中的 value / 语义 |
|---|---:|---:|---|---|
| `REG_DMA_CH_ENABLE` | `0x000` | `0x800` | 通道使能 | `BIT(0)` 启动 |
| `REG_DMA_CH_DIRECTION` | `0x004` | `0x804` | 方向 / 模式控制 | 同 RD |
| `REG_DUMMY_CH_ADDR_L/H` | `0x008 / 0x00C` | `0x808 / 0x80C` | dummy read 地址 | 同 RD |
| `REG_DMA_CH_MMU_ADDR_TYPE` | `0x010` | `0x810` | MMU 地址类型 | 同 RD |
| `REG_DMA_CH_FC` | `0x020` | `0x820` | flow control | 同 RD |
| `0x050` | `0x850` | `dma_wch_pagefault_raw` | WR pagefault raw / status | 脚本命名为 `dma_wch_mmu_pagefault` |
| `0x054` | `0x854` | `dma_wch_pagefault_imsk` | WR pagefault mask | 脚本打印 |
| `0x058` | `0x858` | `dma_wch_pagefault_sts` | WR pagefault status | 脚本打印 |
| `0x05C` | `0x85C` | `dma_wch_pagefault_vpage_h` | WR fault vpage 高位 | 脚本打印 |
| `0x060` | `0x860` | `dma_wch_pagefault_vpage_l` | WR fault vpage 低位 | 脚本打印 |
| `REG_DMA_CH_INTR_IMSK` | `0x0C4` | `0x8C4` | 通道中断 mask | 当前多处写 `0` |
| `REG_DMA_CH_INTR_RAW` | `0x0C8` | `0x8C8` | 通道中断 raw | ISR ack 用 |
| `REG_DMA_CH_INTR_STATUS` | `0x0CC` | `0x8CC` | 通道中断状态 | PF / VF ISR 用来先判定 done |
| `REG_DMA_CH_STATUS` | `0x0D0` | `0x8D0` | 通道状态 | `BIT(0)` = busy |
| `REG_DMA_CH_LBAR_BASIC` | `0x0D4` | `0x8D4` | 链表基本参数 | 同 RD |
| `REG_DMA_CH_DESC_OPT` | `0x400` | `0xC00` | 第 0 个描述符 | 同 RD |
| `REG_DMA_CH_ACNT` | `0x404` | `0xC04` | 计数字段 | 同 RD |
| `REG_DMA_CH_SAR_L/H` | `0x408 / 0x40C` | `0xC08 / 0xC0C` | 源地址 | 同 RD |
| `REG_DMA_CH_DAR_L/H` | `0x410 / 0x414` | `0xC10 / 0xC14` | 目的地址 | 同 RD |
| `REG_DMA_CH_LAR_L/H` | `0x418 / 0x41C` | `0xC18 / 0xC1C` | next desc 地址 | 同 RD |

---

## 4. Interrupt 寄存器表

### 4.1 Channel 级中断寄存器

| 寄存器 | 偏移 | 作用 | 关系 |
|---|---:|---|---|
| `REG_DMA_CH_INTR_IMSK` | `0x0C4` | channel 中断 mask | 控制该 channel 中断是否上报 |
| `REG_DMA_CH_INTR_RAW` | `0x0C8` | channel 原始中断状态 | 真正用于 ack；写回原值清中断 |
| `REG_DMA_CH_INTR_STATUS` | `0x0CC` | channel 中断状态 | 上层先用它筛查，再进 `dma_bare_isr()` 读 RAW |
| `REG_DMA_CH_STATUS` | `0x0D0` | channel 工作状态 | 非中断寄存器，但常用于状态判定 |

### 4.2 `REG_DMA_CH_INTR_RAW / STATUS` 位定义

| 位 | 宏 | 含义 |
|---|---|---|
| bit0 | `DMA_CH_INTR_BIT_DONE` | 传输完成 |
| bit1 | `DMA_CH_INTR_BIT_ERR_DATA` | 数据错误 |
| bit2 | `DMA_CH_INTR_BIT_ERR_DESC_READ` | 描述符读取错误 |
| bit3 | `DMA_CH_INTR_BIT_ERR_CFG` | 配置错误 |
| bit4 | `DMA_CH_INTR_BIT_ERR_DUMMY_READ` | dummy read 错误 |

### 4.3 Common 级 merge interrupt 寄存器

| 寄存器 | 偏移 | 作用 | 关系 |
|---|---:|---|---|
| `REG_DMA_COMM_RD_MRG_PF0_IMSK_C32` | `0xC20` | PF0 读通道 merge mask[31:0] | 控制哪些读通道可汇总上报 |
| `REG_DMA_COMM_RD_MRG_PF0_IMSK_C64` | `0xC24` | PF0 读通道 merge mask[63:32] | 同上 |
| `REG_DMA_COMM_RD_MRG_PF0_STS_C32` | `0xC30` | PF0 读通道 merge status[31:0] | 指出哪些 RD channel 触发了中断 |
| `REG_DMA_COMM_RD_MRG_PF0_STS_C64` | `0xC34` | PF0 读通道 merge status[63:32] | 同上 |
| `REG_DMA_COMM_WR_MRG_PF0_IMSK_C32` | `0xC40` | PF0 写通道 merge mask[31:0] | 控制哪些写通道可汇总上报 |
| `REG_DMA_COMM_WR_MRG_PF0_IMSK_C64` | `0xC44` | PF0 写通道 merge mask[63:32] | 同上 |
| `REG_DMA_COMM_WR_MRG_PF0_STS_C32` | `0xC50` | PF0 写通道 merge status[31:0] | 指出哪些 WR channel 触发了中断 |
| `REG_DMA_COMM_WR_MRG_PF0_STS_C64` | `0xC54` | PF0 写通道 merge status[63:32] | 同上 |
| `REG_DMA_COMM_MRG_PF0_IMSK` | `0xC70` | PF0 merge 总 mask | 屏蔽读汇总 / 写汇总这两个大类 |
| `REG_DMA_COMM_MRG_PF0_STS` | `0xC74` | PF0 merge 总状态 | 当前代码只处理 bit0 与 bit16 |

### 4.4 `REG_DMA_COMM_MRG_PF0_STS` 当前代码用法

| 位 | 当前含义 |
|---|---|
| bit0 | 有 RD merge 中断 |
| bit16 | 有 WR merge 中断 |

### 4.5 中断寄存器关系

#### PF 路径

1. 硬件先产生 channel 中断，`INTR_RAW / INTR_STATUS` 置位。  
2. DMA Common 再把多个 channel 中断汇总：  
   - RD 汇总到 `RD_MRG_PF0_STS_C32 / C64`  
   - WR 汇总到 `WR_MRG_PF0_STS_C32 / C64`  
3. PF 总汇总寄存器 `MRG_PF0_STS` 再告诉驱动是读汇总还是写汇总。  
4. 驱动流程：  
   - 先看 `MRG_PF0_STS`  
   - 再读 `RD / WR_MRG_PF0_STS_C32 / C64` 找到具体 channel bitmap  
   - 再去读对应 channel 的 `REG_DMA_CH_INTR_STATUS`  
   - 最后调用 `dma_bare_isr()` 读 / 清 `REG_DMA_CH_INTR_RAW`  

#### VF 路径

VF 路径没有走 PF 这套 merge bitmap 解码，而是直接固定检查：

- `0x3000 + REG_DMA_CH_INTR_STATUS`（VF RD）
- `0x3800 + REG_DMA_CH_INTR_STATUS`（VF WR）

---

## 5. MMU / OSID / Context / Page Fault 寄存器表

### 5.1 MMU 地址类型

| 寄存器 | 偏移 | 作用 | 当前编码 |
|---|---:|---|---|
| `REG_DMA_CH_MMU_ADDR_TYPE` | `0x010` | 指示本次 DMA 的地址类型 / 访问方向 | 当前驱动直接写 4 个离散编码：`0x0=MEM_TO_MEM`, `0x100=MEM_TO_DEV`, `0x1=DEV_TO_MEM`, `0x101=DEV_TO_DEV`；从取值形态看更像按位组合，而不是连续枚举值 |

### 5.2 Page Fault 类

#### RD Channel Page Fault Registers

| 偏移 | 含义 |
|---:|---|
| `0x050` | pagefault raw |
| `0x054` | pagefault mask |
| `0x058` | pagefault status |
| `0x05C` | fault virtual page high |
| `0x060` | fault virtual page low |

#### WR Channel Page Fault Registers

| 偏移 | 含义 |
|---:|---|
| `0x850` | pagefault 原始事件寄存器（脚本命名为 `dma_wch_mmu_pagefault`，语义偏 raw/summary，和 `0x858` 的 status 分开看更稳妥） |
| `0x854` | pagefault mask |
| `0x858` | pagefault status |
| `0x85C` | fault virtual page high |
| `0x860` | fault virtual page low |

### 5.3 OSID / Context 相关

| 寄存器 / 区域 | 偏移 | 当前代码中的表现 | 说明 |
|---|---:|---|---|
| `REG_DMA_COMM_COMM_ENABLE` | `0x010` | 注释里提示 `BIT(0)` 可能是 `osid_en` | OSID 总开关候选 |
| `REG_DMA_COMM_OSID_SUPER` | `0x014` | 定义了但未使用 | OSID 的 supervisor / 特权控制候选 |
| `REG_DMA_COMM_CH_OSID(i)` | `0x020 + 4 * i` | 代码写 `0x1`，但脚本打印名是 `dma_ch_secure` | 该组寄存器存在 “OSID / secure” 命名冲突 |
| `0x2000 + 4 * i` | `dma_ch_osid`（脚本命名） | 初始化按通道索引写入对应编号 | 更像每通道实际 OSID 值表 |
| `0x2800 + 4 * i` | `dma_ch_context_id`（脚本命名） | 只打印 | 更像与 MMU context 绑定 |

### 5.4 备注

- 旧驱动中，OSID 相关寄存器存在命名不一致现象。  
- 综合初始化写法、脚本打印名和平台 MMU 映射寄存器命名，OSID 可以先理解为 DMA 请求的源身份 / 隔离域标签。  
- 从当前行为看，`0x2000 + 4 * i` 更像 per-channel OSID 值表，而 `0x020 + 4 * i` 更像安全属性或使能属性。  
