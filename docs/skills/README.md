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

## 继承边界

- **允许继承**：文档中定义的架构、接口、参数契约、测试分类、调用顺序、文件职责。
- **禁止继承**：本仓库原始 `.c/.cc/.h` 代码、脚本实现细节、`examples/` 中的任何示例代码。

## 人工维护建议

- 每次接口、IOCTL、测试标签、设备节点、构建变量变更后，先更新对应 skill，再更新代码。
- 每份 skill 都显式列出了关键接口和其来源文件，方便人工按文件差异逐项同步。
