# 架构说明

本文面向贡献者，描述 `hash` 仓库的模块划分、关键数据流与约定。用户视角的功能介绍见 [README](../README.md)。

## 模块地图

```
src/
├── core/    仅依赖 Qt6::Core 的流式哈希引擎
│   ├── registry.*      算法注册表：名称/标签/类别 → Digester 工厂
│   ├── digesters.*     内置增量摘要器（Adler-32、CRC、FNV、SHA-512/224、SHA-512/256）
│   ├── hasher.*        单文件哈希：一次读取扇出给全部算法、TOCTOU 安全、Changed 检测
│   │                   hasher_windows.cpp / hasher_posix.cpp 提供平台文件句柄实现
│   ├── batch.*         批量编排：worker 池、保序结果、进度快照、FailFast
│   └── types.h         FileRequest / Result / FileResult / Progress
├── common/  引擎与入口共用的工具
│   ├── atomicfile.*    原子输出：临时文件 + rename，拒绝符号链接目标与输入文件
│   ├── pathkey.*       平台感知的路径键（仅 Windows 折叠大小写）
│   ├── compare.*       摘要清单解析与比对（见下文「比对管线」）
│   ├── quoting.*       Go 风格引号串编解码
│   └── fspath.*        std::filesystem ↔ QString 转换
├── cli/     控制台入口：options（参数解析）、paths（glob 展开去重）、format（text/tsv/json）
├── tools/   QtCore-only 静态库 hash_tools：GUI 的全部后端模型
│   ├── hashcontroller.*  批量会话控制器（队列、进度、输出、比对），QML 单例
│   ├── batchworker.h     专用线程上的 hashFiles 调用者，100 ms 进度节流
│   ├── hashfilemodel.*   文件队列模型（状态、摘要、错误）
│   ├── algorithmsmodel.* / toolregistrymodel.*  算法与工具目录模型
│   └── textdigesttool / codectool / jsontool / timestamptool /
│       uuidtool / radixtool / colortool / passwordtool   每工具一个小控制器
└── app/     Qt Quick GUI（hash-gui）
    ├── main.cpp        入口：暗色调色板、Basic 样式、单例注册、诊断日志
    ├── Main.qml        启动器外壳（命令栏、工具网格、StackView、状态栏）
    ├── Theme.qml       颜色/圆角/动画令牌单例 —— 全部自定义表面唯一取色处
    ├── Toast.qml       跨页通知通道（状态栏消费）
    ├── AppScrollBar.qml / AlgorithmChips.qml / PageHeader.qml   共享控件
    └── *Page.qml       各工具页（懒加载）
```

静态库 `hash_core`、`hash_tools` + 可执行 `hash`、`hash-gui`、`hash_tests` 的定义见顶层 `CMakeLists.txt` 与 `src/app/CMakeLists.txt`。

## 批量哈希管线

`FileHashPage` → `HashController::start(algorithms)` → 队列重置 → `BatchWorker::run`（专用 QThread，队列调用）→ `hashFiles`。

- `hashFiles`（`core/batch.cpp`）以 `nextIndex` 原子计数分发文件给 `min(4, 硬件并发)` 个 `std::thread`；结果按请求顺序写回。Windows 上这些 worker 以 `THREAD_PRIORITY_BELOW_NORMAL` 运行，满载计算不抢占 UI 与其他程序。
- 进度语义由 `ProgressTracker` 保证：互斥锁内只改状态并取快照，用户回调在锁外触发（可能并发）。文件开始时 `CurrentBytes` 归零，完成时计入 `CompletedBytes` 并归零；`CompletedBytes + CurrentBytes` 是已哈希字节数。
- 块级进度两级节流：`hasher` 每 1 MiB 读取回调一次，`batch` 每 8 MiB 才转发给 tracker/回调（`kProgressChunkBytes`），`BatchWorker` 再按 100 ms 节流向 GUI 线程发 queued signal。三级叠加后事件循环不会被大队列冲垮，UI 以约 10 Hz 刷新。
- 取消是共享的 `std::atomic<bool>`；已取消文件记为 canceled 状态并保留已完成结果。`FailFast` 时任一错误即置取消标志。
- `hasher` 先 `Open` 再对句柄 stat（句柄是权威对象，无 `Lstat→Open` TOCTOU 窗口），跟随符号链接、拒绝非普通文件；结束后对句柄与路径各 stat 一次，结合大小/mtime 产出 `changed`，GUI 提示「已变化」。

## 比对管线

`HashController::compareClipboard(text)` → `parseDigestText`（`common/compare.cpp`）→ `compareRecords` → `compare` 属性 → 页面显示五项计数。

`parseDigestText` 接受两类输入：

1. **TSV 记录**（≥3 个制表符字段）：`算法 摘要 路径 [错误]`，兼容 BOM、表头、`WARNING:`/`ERROR` 行、Go 引号路径、TSV 转义与公式注入防护前缀（`'`）。
2. **宽松摘要行**（其余行）：`[算法[:：=] ]摘要 [路径]`。算法标签（如 `sha256:`、`SHA-256：`、`md5 =`，半角/全角冒号或等号分隔）自动剥离；摘要须为 8–128 位偶数长度十六进制，否则报「not a digest record」。这类记录标记为 `loose`，路径仅为展示保留。

`compareRecords` 的匹配规则：

- 完整记录（非 loose）按 `算法 + pathKey(路径)` 键匹配，产生 匹配/不匹配/缺少/多余/重复 计数；
- loose 记录没有可信的算法与路径，仅按摘要值与对方全量摘要集合比对：命中计匹配、未命中计不匹配，不影响其他计数；
- TSV 摘要字段内嵌的算法标签同样会被剥离（例如 `sha256<TAB>SHA-256:ABC<TAB>…`）。

## 线程模型

- GUI 线程只与 `HashController` 及各工具控制器交互；`BatchWorker` 通过 `QMetaObject::invokeMethod` 队列调用，进度/完成经 queued signal 回 GUI 线程。
- `BatchWorker` 的进度 lambda 可能被多个 worker 线程并发调用，节流状态由互斥锁保护。
- `HashController` 析构时置取消标志并等待线程退出；运行中关窗安全。

## 界面一致性约定

- 所有自定义表面的颜色、圆角、动画时长一律取自 `Theme.qml` 令牌，禁止在页面里写裸色值；`main.cpp` 的 QPalette 与令牌保持同值镜像，驱动 Basic 样式的控件（含由 `Mid`/`Light`/`Dark` 派生的边框）。
- 状态颜色统一走 `Theme.statusColor(status)`；共享控件（`PageHeader`、`AlgorithmChips`、`AppScrollBar`）保证各页面结构一致。
- 算法名称是小写注册表键（`sha256`、`crc32-ieee`）；页面展示用 `AlgorithmsModel` 的标签。

## 测试与 CI

- `tests/` 为 Qt Test：一个 `test_*.h` 一个 `QObject` 用例类，`tests/main.cpp` 注册；文件系统夹具用 `QTemporaryDir`，平台差异用 `Q_OS_WIN`/`QSKIP` 处理。
- `.github/workflows/ci.yml`：clang-format 检查 + Ubuntu/Windows 双平台构建测试 + headless GUI 冒烟（`QT_QPA_PLATFORM=offscreen`，退出码 124 即超时视为启动成功）。
- `.github/workflows/release.yml`：`v*` tag 触发 Windows 构建，`windeployqt` 裁剪部署（跳过 imageformats/networkinformation/tls 插件），7z 最高压缩打包 zip 并生成 SHA-256 校验文件。
- 仓库约定：本地一律不构建、不运行、不测试；一切验证交给 GitHub Actions。
