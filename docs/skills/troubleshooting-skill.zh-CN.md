---
title: "Skill：PCIe & DMA 构建/加载/测试排障"
version: "1.0"
last_modified: "2026-03-17"
dependencies:
  - "environment-skill.zh-CN.md"
  - "driver-skill.zh-CN.md"
  - "test-suite-skill.zh-CN.md"
target_audience: "AI Agent / Project Replicator"
status: "usable"
source_scope:
  - "README.md"
  - "driver/"
  - "test/"
  - "scripts/"
excludes:
  - "examples/"
---

# Skill：PCIe & DMA 构建/加载/测试排障

## 1. 目标

这份 skill 不重复定义主 skill 中的架构与接口，而是把“**出了问题先查哪里**”集中整理成故障树，方便：

- AI 工具快速定位应读哪份主 skill
- 人工在新环境落地时缩短试错路径
- 把“当前仓库已知不兼容点”与“运行期排查动作”分开管理

---

## 2. 先分问题类型

遇到问题时，先判断属于哪一类：

1. **driver 编译失败**
   - 先看 `environment-skill.zh-CN.md` §8.1
2. **test 编译失败**
   - 先看 `environment-skill.zh-CN.md` §8.2
   - 再看 `test-suite-skill.zh-CN.md` §12
3. **模块能编译但加载失败**
   - 先看 `environment-skill.zh-CN.md` §2.4 / §4.4 / §4.5
4. **设备节点或 sysfs 不完整**
   - 先看 `environment-skill.zh-CN.md` §5
5. **DMA / 中断 / target 命令运行失败**
   - 先看 `driver-skill.zh-CN.md` §8 ~ §10
   - 再看 `test-suite-skill.zh-CN.md` §4 ~ §7

---

## 3. driver 编译问题

### 3.1 `iommu_map` 参数不匹配

当前仓库在本地内核头 `6.14.0-1017-azure` 下，`driver/make` 会命中：

- `iommu_map` 参数个数不匹配

这说明旧驱动依赖的 IOMMU API 与新内核头不兼容。处理原则：

1. 不要把“当前仓库可直接编过”当作前提
2. 把内核版本当环境参数
3. 优先在 environment skill 中记录兼容矩阵，再决定是否做代码适配

### 3.2 `pci_enable_pcie_error_reporting` 未声明

同样属于内核 API / 配置差异，而不是纯业务逻辑问题。结论：

- 新项目如果重建驱动，AER 相关能力应做成**可选能力**
- 不应把这类接口存在性写死为“默认必可用”

### 3.3 如果 `driver/make` 一开始就失败，先别继续做测试

排障顺序必须是：

1. driver 能编译
2. 模块能按顺序加载
3. `/dev` 与 `/sys/class/misc` 元数据完整
4. 再进入测试与性能问题

---

## 4. test 编译问题

### 4.1 Catch2 老版本兼容问题

当前仓库在本地工具链下，已有明确基线问题：

- `sysconf()` constexpr 报错
- `altStackMem` 不是 integral constant expression

这意味着 test skill 的真正依赖不只是源码，还包括：

- Catch2 版本
- C++ 标准
- 编译器版本

### 4.2 如何判断是“测试框架问题”还是“业务测试代码问题”

经验规则：

- 如果错误落在 `test/src/catch2/catch.hpp`，优先视为框架兼容问题
- 如果错误落在 `test/lib/mt_pcie_f.c` / `test/src/*.cc`，再判断是否是业务逻辑/头文件/类型问题

### 4.3 先通过配置，再看编译

当前基线里：

- `cmake ..` 可以完成
- 真正问题主要出在后续 `make`

所以新环境中应分开记录：

1. CMake 配置是否成功
2. 编译是否成功
3. 失败点属于外部依赖还是项目代码

---

## 5. 模块加载与设备发现问题

### 5.1 模块加载顺序错误

固定顺序来源于 `scripts/install.sh` 与 environment skill：

```bash
sudo insmod driver/mt_emu_mtdma.ko
sudo insmod driver/mt_emu_gpu.ko
sudo insmod driver/mt_emu_apu.ko
sudo insmod driver/mt_emu_vgpu.ko
```

如果顺序打乱，最常见后果是：

- 依赖 symbol 尚未准备好
- 后续设备节点不全
- VF 侧测试初始化失败

### 5.2 `/dev/mt_emu_gpu` 或 `/dev/mt_emu_dmabuf` 不出现

优先检查：

1. 模块是否全部加载成功
2. `probe()` 是否执行成功
3. misc 设备是否注册成功
4. sysfs 中是否存在对应 `bar*` 与 `vf` 元数据

### 5.3 `pcief_init()` 失败

`pcief_init()` 的前提不是“设备文件存在”这么简单，而是同时要求：

- 能打开 `/dev/mt_emu_gpu`
- 能打开 `/dev/mt_emu_dmabuf`
- 能从 `/sys/class/misc/<device>/bar0` / `bar2` 读取 BAR 元数据
- BAR 能被 `mmap`

也就是说，**只修复 `/dev` 不够，sysfs 元数据路径也必须保持一致**。

---

## 6. DMA / 中断 / target 命令问题

### 6.1 bare DMA 超时

优先排查四件事：

1. `pcief_irq_init()` 是否先完成
2. `pcief_dmaisr_set()` 是否选对 bare / engine 模式
3. 传入的 `timeout_ms` 是否明显过小
4. `driver-skill.zh-CN.md` §9 / §10 中的描述符和完成同步逻辑是否被正确重建

### 6.2 target 命令失败

如果 `pcief_tgt_*` 失败，不要只盯着单个 helper。应回到更高层抽象检查：

1. 主机侧命令/响应槽写读是否完整
2. `pcief_tgt_cmd()` 的等待完成逻辑是否可用
3. target 枚举（DSP/FEC/SMC）是否与实际路由一致

### 6.3 `pcief_tgt_mtdma_reset()` 失败

这个接口在 `test/src/mthdma.cc` 的 `dma_reset` 用例里被直接使用，说明它不只是调试接口，而是回归测试路径的一部分。若失败，应分别判断：

- target 命令通道整体失败
- 只有 reset 命令不支持
- MTDMA 本体状态已经异常

### 6.4 性能异常但功能没坏

若 DMA 功能通了但带宽异常，先分情况：

- 功能 correctness 问题：优先看测试 helper 与描述符参数
- 性能问题：优先看 `pcief_perf_*`、链路速率、通道并行度、MMU/缓存设置

---

## 7. AI 重建时最容易踩的坑

### 7.1 只看 QUICK_START，不回主 skill 校准细节

`QUICK_START.zh-CN.md` 只负责固定顺序，不替代主 skill 的细节约束。碰到以下问题必须回主文档：

- 结构体字段
- IOCTL 编号
- target 接口
- 线程化参数结构

### 7.2 把 `examples/` 当成默认来源

当前 skills 明确排除 `examples/`。除非人类明确要求，否则：

- 不要用 `examples/` 补 driver/test 契约
- 不要把示例代码的实现细节当作主 skill 的默认来源

### 7.3 看到旧接口命名不规范就擅自“修正”

例如：

- `pcief_get_vf__num()`
- `scripts/bulid.sh`

skill 文档必须先保留“**当前仓库真实契约**”，再额外注明命名质量问题，不能直接在文档里擅自改掉接口名。

---

## 8. 推荐排障顺序

按以下顺序最省时间：

1. 先确认当前问题属于 build / load / discover / runtime 哪一类
2. 再回到对应主 skill 章节
3. 只有主 skill 确认无缺口时，才补新兼容性说明
4. 兼容性问题优先补 `environment-skill`
5. 公开接口变化优先补 `driver-skill` 或 `test-suite-skill`

---

## 9. 人工更新检查表

- [ ] 新内核版本是否引入新的 driver 编译失败模式
- [ ] 新编译器 / 新 Catch2 版本是否引入新的 test 编译失败模式
- [ ] 模块加载顺序是否变化
- [ ] 设备发现路径（`/dev` / `/sys/class/misc`）是否变化
- [ ] 新增 target 命令 / DMA / shell 调试入口是否已补入主 skill
