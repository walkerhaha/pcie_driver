---
title: "PCIe & DMA Skills Index"
version: "1.1"
last_modified: "2026-03-17"
target_audience: "AI Agent / Project Replicator"
status: "usable"
scope: "driver,test,scripts without examples"
---

# PCIe & DMA Skills 索引

这些 skills 以当前仓库 `driver/`、`test/`、`scripts/` 的实现为依据整理，**明确排除 `examples/` 目录及其内容**。目标不是复用本仓库代码，而是让其它 AI 工具仅参考这些文档，也能从零重建：

- PCIe & DMA 驱动
- 编译/加载/测试环境
- 测试集与参数化方法

## Skills 列表

1. [`driver-skill.zh-CN.md`](./driver-skill.zh-CN.md)
   - 定义驱动架构、关键文件、关键函数、IOCTL 契约、PF/VF 分工、DMA 中断与数据流。
2. [`environment-skill.zh-CN.md`](./environment-skill.zh-CN.md)
   - 定义构建、加载、运行、测试环境准备方式，以及需要外部传入的全部关键参数。
3. [`test-suite-skill.zh-CN.md`](./test-suite-skill.zh-CN.md)
   - 定义测试入口、标签体系、关键测试 API、线程化参数、测试集组织与扩展方式。
4. [`QUICK_START.zh-CN.md`](./QUICK_START.zh-CN.md)
   - 给出只依赖这些 skills 文档即可重建驱动、环境、测试集的固定消费顺序。
5. [`troubleshooting-skill.zh-CN.md`](./troubleshooting-skill.zh-CN.md)
   - 把构建、加载、设备发现、DMA/target 运行时问题整理为集中排障入口。
6. [`source-to-skill.mapping.zh-CN.md`](./source-to-skill.mapping.zh-CN.md)
   - 提供源码文件 / API / 数据结构到各份 skill 章节的快速跳转索引。

## 内部导航与继承顺序

### 快速路径：只想先搭环境并跑测试

1. 先读 [`environment-skill.zh-CN.md`](./environment-skill.zh-CN.md) 的：
   - §3 必须外部传入的环境参数
   - §4 环境准备流程
   - §7 测试运行参数契约
2. 再读 [`test-suite-skill.zh-CN.md`](./test-suite-skill.zh-CN.md) 的：
   - §4 必须显式列出的关键测试接口
   - §6 bare DMA 参数化规则
   - §8 测试标签体系

### 完整路径：从零重建 PCIe & DMA 驱动体系

1. 环境参数：[`environment-skill.zh-CN.md`](./environment-skill.zh-CN.md) §3
2. 构建/加载流程：[`environment-skill.zh-CN.md`](./environment-skill.zh-CN.md) §4 ~ §6
3. 驱动总体分层：[`driver-skill.zh-CN.md`](./driver-skill.zh-CN.md) §2 ~ §3
4. 驱动关键结构与常量：[`driver-skill.zh-CN.md`](./driver-skill.zh-CN.md) §4 ~ §5
5. 驱动核心接口与 IOCTL：[`driver-skill.zh-CN.md`](./driver-skill.zh-CN.md) §6 ~ §8
6. DMA 传输与中断内核逻辑：[`driver-skill.zh-CN.md`](./driver-skill.zh-CN.md) §9 ~ §10
7. 用户态测试 API：[`test-suite-skill.zh-CN.md`](./test-suite-skill.zh-CN.md) §4
8. 参数化测试集组织：[`test-suite-skill.zh-CN.md`](./test-suite-skill.zh-CN.md) §5 ~ §11
9. 最后按 [`QUICK_START.zh-CN.md`](./QUICK_START.zh-CN.md) 的固定步骤落地

## 场景化导读路径

### 场景 1：先搭环境并跑冒烟测试

1. [`environment-skill.zh-CN.md`](./environment-skill.zh-CN.md) §3 ~ §5
2. [`test-suite-skill.zh-CN.md`](./test-suite-skill.zh-CN.md) §3 ~ §4.5
3. [`QUICK_START.zh-CN.md`](./QUICK_START.zh-CN.md) §2 ~ §5

### 场景 2：只关心 bare DMA / 中断链路

1. [`driver-skill.zh-CN.md`](./driver-skill.zh-CN.md) §9 ~ §10
2. [`test-suite-skill.zh-CN.md`](./test-suite-skill.zh-CN.md) §4.4 ~ §4.5、§6 ~ §7
3. 出问题时跳到 [`troubleshooting-skill.zh-CN.md`](./troubleshooting-skill.zh-CN.md) §6

### 场景 3：只关心 target / SMC / DSP 命令链路

1. [`test-suite-skill.zh-CN.md`](./test-suite-skill.zh-CN.md) §4.4、§4.7、§4.8
2. [`source-to-skill.mapping.zh-CN.md`](./source-to-skill.mapping.zh-CN.md) §3 ~ §5
3. 出问题时跳到 [`troubleshooting-skill.zh-CN.md`](./troubleshooting-skill.zh-CN.md) §6.2 ~ §6.3

### 场景 4：只想定位源码该看哪份文档

1. 先读 [`source-to-skill.mapping.zh-CN.md`](./source-to-skill.mapping.zh-CN.md)
2. 再回到对应主 skill

### 场景 5：新环境编译失败，先做兼容性排查

1. [`environment-skill.zh-CN.md`](./environment-skill.zh-CN.md) §8
2. [`test-suite-skill.zh-CN.md`](./test-suite-skill.zh-CN.md) §12
3. [`troubleshooting-skill.zh-CN.md`](./troubleshooting-skill.zh-CN.md) §3 ~ §4

### 痛点快查表

| 要解决的问题 | 优先查阅 |
|---|---|
| 驱动有哪些层、文件该怎么拆 | `driver-skill.zh-CN.md` §2 ~ §3 |
| PF/VF、dmabuf、IOCTL 怎样串起来 | `driver-skill.zh-CN.md` §6 ~ §8 |
| bare DMA 描述符和中断流程 | `driver-skill.zh-CN.md` §9 ~ §10 |
| 编译参数和运行时可变参数 | `environment-skill.zh-CN.md` §3 |
| 驱动加载和链路切速/重扫 | `environment-skill.zh-CN.md` §4 ~ §6 |
| 测试侧有哪些 API 可以当契约 | `test-suite-skill.zh-CN.md` §4 |
| DMA 测试如何参数化 | `test-suite-skill.zh-CN.md` §5 ~ §7 |
| 测试标签怎么分层组织 | `test-suite-skill.zh-CN.md` §8 ~ §9 |
| 源码文件对应哪份 skill | `source-to-skill.mapping.zh-CN.md` |
| 编译/加载/运行失败先怎么排 | `troubleshooting-skill.zh-CN.md` |

## 继承边界

- **允许继承**：文档中定义的架构、接口、参数契约、测试分类、调用顺序、文件职责。
- **禁止继承**：本仓库原始 `.c/.cc/.h` 代码、脚本实现细节、`examples/` 中的任何示例代码。

## Skill 元数据说明

三份主 skill 顶部都带统一元数据头，方便其它 AI 工具做自动选择：

- `title`：skill 名称
- `version`：文档版本
- `last_modified`：最后整理时间
- `dependencies`：依赖的其它 skill
- `target_audience`：目标消费方
- `maintenance_checklist_complete`：是否已完成该文档内部检查表
- `status`：可用状态
- `source_scope`：本 skill 允许吸收的源码范围
- `excludes`：明确排除范围

读取这些元数据后，AI 工具可以先判断应该先看哪份文档，再决定是否需要串读其它 skill。

## 人工维护建议

- 每次接口、IOCTL、测试标签、设备节点、构建变量变更后，先更新对应 skill，再更新代码。
- 每份 skill 都显式列出了关键接口和其来源文件，方便人工按文件差异逐项同步。
