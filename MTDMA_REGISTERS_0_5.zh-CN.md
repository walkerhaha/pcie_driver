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
| `REG_DMA_COMM_COMM_ENABLE` | `0x010` | 一些Common 功能总开关，当前只定义了osid_en | `BIT(0)` 是 `osid_en`，当前未开启 |
| `REG_DMA_COMM_OSID_SUPER` | `0x014` | OSID super 配置 | 暂不配置 |
| `REG_DMA_COMM_CH_OSID` | `0x020` | 配置chain id | 配置`cfg_ch_prot`，`cfg_ch_prot`，`cfg_ch_hyper`，`cfg_ch_user`等寄存器，暂不配置 |
| `REG_DMA_COMM_CC_MODE_DISBALE` | `0x018` | cc mode 使能控制 | 暂不配置 |
| `REG_DMA_COMM_CH_NUM` | `0x400` | 通道数量配置 | 设为全局可配 |
| `REG_DMA_COMM_MST0_BLEN` | `0x408` | MST0 burst 长度 | 设为全局可配 |
| `REG_DMA_COMM_MST0_CACHE` | `0x420` | MST0 cache 属性 | 暂不配置 |
| `REG_DMA_COMM_MST0_PROT` | `0x424` | MST0 prot 属性 | 暂不配置 |
| `REG_DMA_COMM_MST2_CACHE` | `0x428` | MST2 cache 属性 | 暂不配置 |
| `REG_DMA_COMM_MST1_BLEN` | `0x608` | MST1 burst 长度 | 设为全局可配 |
| `REG_DMA_COMM_MST1_CACHE` | `0x620` | MST1 cache 属性 | 暂不配置 |
| `REG_DMA_COMM_MST1_PROT` | `0x624` | MST1 prot 属性 | 暂不配置 |
| `REG_DMA_COMM_MST3_CACHE` | `0x628` | MST3 cache 属性 | 暂不配置 |

### 1.2 Common 状态 / 汇总类

| 名称 | 偏移 | 含义 | 说明 |
|---|---:|---|---|
| `REG_DMA_COMM_WORK_STS` | `0xD00` | DMA 工作状态 | 初始化后会检查是否 busy |
| `REG_DMA_COMM_RCH_FC` | `0x1000` | RD channel flow control 参数 | 暂不配置 |
| `REG_DMA_COMM_WCH_FC` | `0x1800` | WR channel flow control 参数 | 暂不配置 |
| `REG_DMA_ALLOC_CH_OSID` | `0x2000` | DMA channel OSID 配置寄存器 | 暂不配置 |
| `REG_DMA_ALLOC_CH_CONTEXT_ID` | `0x2800` | DMA context_id 配置寄存器 | 暂不配置 |

---

## 2. RD Channel 寄存器表

> RD chN 基址：`channel_base_addr + N * 0x1000`

| 名称 | 相对 RD 基址偏移 | 含义 | 当前驱动中的 value / 语义 |
|---|---:|---|---|
| `REG_DMA_RCH_ENABLE` | `0x000` | 通道使能 | `BIT(0)` 启动 |
| `REG_DMA_RCH_DIRECTION` | `0x004` | 方向 / 模式控制 | 见后文“方向编码” |
| `REG_DUMMY_RCH_ADDR_L` | `0x008` | dummy read 地址低 32 位 | dummy read使能时读取 |
| `REG_DUMMY_RCH_ADDR_H` | `0x00C` | dummy read 地址高 32 位 | dummy read使能时读取 |
| `REG_DMA_RCH_MMU_ADDR_TYPE` | `0x010` | MMU 地址类型 | `BIT(0)`配置rd ch src addr type；`BIT(8)`配置rd ch dst addr type；`BIT(16)`配置link block addr type。配置为0是physical addr，配置为1是virtual addr |
| `REG_DMA_RCH_PAGEFAULT_RAW` | `0x050` | RD pagefault raw | 后续统一分析 |
| `REG_DMA_RCH_PAGEFAULT_IMSK` | `0x054` | RD pagefault mask | 后续统一分析 |
| `REG_DMA_RCH_PAGEFAULT_STS` | `0x058` | RD pagefault status | 后续统一分析 |
| `REG_DMA_RCH_PAGEFAULT_VPAGE_H` | `0x05C` | RD fault vpage 高位 | 后续统一分析 |
| `REG_DMA_RCH_PAGEFAULT_VPAGE_L` | `0x060` | RD fault vpage 低位 | 后续统一分析 |
| `REG_DMA_RCH_INTR_IMSK` | `0x0C4` | 通道中断 mask | 后续统一分析 |
| `REG_DMA_RCH_INTR_RAW` | `0x0C8` | 通道中断 raw / 写回清中断 | ISR 主要读写它，后续统一分析 |
| `REG_DMA_RCH_INTR_STATUS` | `0x0CC` | 通道中断状态 | PF / VF ISR 用来先判定 done，后续统一分析 |
| `REG_DMA_RCH_STATUS` | `0x0D0` | 通道状态 | `BIT(0)` = busy |
| `REG_DMA_RCH_LBAR_BASIC` | `0x0D4` | 链表基本参数 | `bits[31:16]=desc 数`, `bit0=chain_en` |
| `REG_DMA_RCH_DESC_OPT` | `0x400` | 第 0 个描述符起始地址 | desc0 放在寄存器区 |
| `REG_DMA_RCH_ACNT` | `0x404` | 计数字段 | 对应 desc 的 `cnt` |
| `REG_DMA_RCH_SAR_L/H` | `0x408 / 0x40C` | 源地址 | desc 字段 |
| `REG_DMA_RCH_DAR_L/H` | `0x410 / 0x414` | 目的地址 | desc 字段 |
| `REG_DMA_RCH_LAR_L/H` | `0x418 / 0x41C` | 下一个描述符地址 | desc 字段 |

---

## 3. WR Channel 寄存器表

> WR chN 基址：`channel_base_addr + N * 0x1000 + 0x800`

| 名称 | 相对 WR 基址偏移 | 绝对窗口偏移示例 | 含义 | 当前驱动中的 value / 语义 |
|---|---:|---:|---|---|
| `REG_DMA_WCH_ENABLE` | `0x000` | `0x800` | 通道使能 | `BIT(0)` 启动 |
| `REG_DMA_WCH_DIRECTION` | `0x004` | `0x804` | 方向 / 模式控制 | 同 RD |
| `REG_DUMMY_WCH_ADDR_L/H` | `0x008 / 0x00C` | `0x808 / 0x80C` | dummy read 地址 | 同 RD |
| `REG_DMA_WCH_MMU_ADDR_TYPE` | `0x010` | `0x810` | MMU 地址类型 | 同 RD |
| `REG_DMA_WCH_PAGEFAULT_RAW` | `0x050` | `0x850` | WR pagefault raw / status | 后续统一分析 |
| `REG_DMA_WCH_PAGEFAULT_IMSK`| `0x054` | `0x854` | WR pagefault mask | 后续统一分析 |
| `REG_DMA_WCH_PAGEFAULT_STS` | `0x058` | `0x858` | WR pagefault status | 后续统一分析 |
| `REG_DMA_WCH_PAGEFAULT_VPAGE_H` | `0x05C` | `0x85C` | WR fault vpage 高位 | 后续统一分析 |
| `REG_DMA_WCH_PAGEFAULT_VPAGE_L` | `0x060` | `0x860` | WR fault vpage 低位 | 后续统一分析 |
| `REG_DMA_WCH_INTR_IMSK` | `0x0C4` | `0x8C4` | 通道中断 mask | 后续统一分析，当前多处写 `0` |
| `REG_DMA_WCH_INTR_RAW` | `0x0C8` | `0x8C8` | 通道中断 raw | 后续统一分析，ISR ack 用 |
| `REG_DMA_WCH_INTR_STATUS` | `0x0CC` | `0x8CC` | 通道中断状态 | 后续统一分析，PF / VF ISR 用来先判定 done |
| `REG_DMA_WCH_STATUS` | `0x0D0` | `0x8D0` | 通道状态 | `BIT(0)` = busy |
| `REG_DMA_WCH_LBAR_BASIC` | `0x0D4` | `0x8D4` | 链表基本参数 | 同 RD |
| `REG_DMA_WCH_DESC_OPT` | `0x400` | `0xC00` | 第 0 个描述符 | 同 RD |
| `REG_DMA_WCH_ACNT` | `0x404` | `0xC04` | 计数字段 | 同 RD |
| `REG_DMA_WCH_SAR_L/H` | `0x408 / 0x40C` | `0xC08 / 0xC0C` | 源地址 | 同 RD |
| `REG_DMA_WCH_DAR_L/H` | `0x410 / 0x414` | `0xC10 / 0xC14` | 目的地址 | 同 RD |
| `REG_DMA_WCH_LAR_L/H` | `0x418 / 0x41C` | `0xC18 / 0xC1C` | next desc 地址 | 同 RD |

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

## 5. 描述符预取和数据传输方向配置

### 5.1 设计框架介绍

DMA在架构上包含两个CORE，分别用来控制RCH和WCH。对外AXI接口包含MST0,MST1,MST2,MST3共四个，其中MST0和MST1是常规读写端口，MST2和MST3用于MMU。MST0用于远端访问，MST1用于近端访问。
它们之间通过CORE SWITCH来调配，确保CORE可以通过合适的端口进行交互。具体是根据描述符存放地址的不同及具体数据搬运方向的需求，来配置相应寄存器。

### 5.2 寄存器用法

`REG_DMA_RCH_DIRECTION`和`REG_DMA_WCH_DIRECTION`分别用来控制RCH及WCH的描述符预取端口和数据传输方向。
具体来讲，`REG_DMA_RCH_DIRECTION`的`BIT(0)`用于控制描述符预取端口，0代表存放在近端，通过MST1进行预取，1代表存放在远端，通过MST0进行预取。`bits[2:1]`用于控制数据传输方向，00和10均是H2D，01和11均是H2H。
`REG_DMA_WCH_DIRECTION`的`BIT(0)`用于控制描述符预取端口，0代表存放在远端，通过MST1进行预取，1代表存放在近端，通过MST0进行预取。`bits[2:1]`用于控制数据传输方向，00和10均是D2H，01和11均是D2D。
