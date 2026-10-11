# Hash 工具箱

一个面向 Windows 的轻量文件工具箱，基于 **C++17 与 Qt 6** 构建，提供 CLI 与 Qt Quick (QML) GUI 两种入口。核心是批量文件哈希校验（参考 `keir.net/hash.html`），并像 uTools 一样内置一组日常小工具：文本哈希、Base64/URL 编解码、JSON、时间戳、UUID、进制转换、颜色转换与密码生成。

## 支持算法

- 密码学哈希：SHA-224、SHA-256、SHA-384、SHA-512、SHA-512/224、SHA-512/256
- 传统兼容：MD5、SHA-1（不适合安全用途）
- 校验和：Adler-32、CRC-32 IEEE/Castagnoli/Koopman、CRC-64 ECMA/ISO
- 非加密哈希：FNV-1/FNV-1a 的 32/64/128 位版本

标准摘要（MD5、SHA 系列）由 Qt 内置的 `QCryptographicHash` 计算；Adler-32、CRC、FNV 与 SHA-512/224、SHA-512/256 为项目内置实现，全部通过标准测试向量校验。文件哈希与文本哈希共用同一套 digester。

## CLI

```text
hash.exe [options] <file-or-pattern>...
```

常用示例：

```text
hash.exe image.iso
hash.exe --algorithm sha256,sha512 --size --modified image.iso
hash.exe --all --format json *.zip
hash.exe --format tsv --output hashes.tsv --recursive .
```

| 选项 | 说明 |
| --- | --- |
| `--algorithm/-a <list>` | 逗号分隔的算法列表（默认 `sha256`） |
| `--all` | 勾选全部注册算法 |
| `--size` / `--modified` / `--utc` | 输出文件大小 / 修改时间 / UTC 时间 |
| `--workers <n>` | 并发 worker 数（默认 `min(4, 硬件并发)`） |
| `--format text\|tsv\|json` | 输出格式（默认 text） |
| `--output/-o <file>` | 结果原子写入指定文件；被 Ctrl+C 中断时不写入，原文件保持不变 |
| `--recursive` / `--literal` / `--fail-fast` | 目录递归 / 关闭通配符 / 首错即停 |
| `--version` / `-h, --help` | 打印版本 / 打印用法（输出到 stdout） |

Windows 下通配符由程序展开；输出按输入顺序返回，文件流式读取，不整载内存。退出码：`0` 成功（含 `--version`、`-h`），`1` 失败，`2` 用法错误，`3` 已取消（Ctrl+C）。

## GUI（工具箱）

`hash-gui.exe` 采用「左侧分类侧栏 + 右侧内容区」布局：侧栏按哈希 / 编解码 / 转换 / 生成分组列出工具，点击即切换；Ctrl+K 或 Ctrl+P 打开命令面板，输入即筛选，↑↓ 选择、回车打开；Esc 返回概览页（工具卡片网格）。工具页首次打开时才创建，之后保持存活，在工具间来回切换不会丢失输入与结果。

内置工具：

- **文件哈希** — 批量计算文件摘要：拖拽或选择文件，多算法单次读取并行计算，实时进度与取消；结果可复制或原子保存
- **文本哈希** — 计算一段文本的任意注册摘要（UTF-8）
- **Base64 / URL 编解码** — 编码、解码与输入输出互换，解码容错空白
- **JSON 格式化** — 格式化、压缩与校验，错误定位到行
- **时间戳** — Unix 秒/毫秒与日期时间互换，本地时区与 UTC 双显示
- **UUID 生成** — 批量生成 UUID v4，可控制大小写、大括号与连字符
- **进制转换** — 2-36 进制整数互换，支持负数
- **颜色转换** — HEX/RGB/HSL 互换，支持 Alpha 通道
- **密码生成** — 按长度与字符集批量生成随机密码（系统安全随机源），可排除易混淆字符

### 哈希比对

文件哈希页的「粘贴对比」支持两种粘贴内容：

1. **完整摘要清单** — 本工具或其他工具生成的 `算法<TAB>摘要<TAB>路径` TSV 文本，逐项比对并给出匹配/不匹配/缺少/多余/重复计数；
2. **裸摘要** — 直接粘贴 `sha256:e3b0c4…`、`SHA-256 = …`、`md5：…` 或不带前缀的十六进制摘要（可附带文件路径）。算法前缀（半角/全角冒号或等号分隔）会自动剥离，摘要按值与本次计算结果比对，无需手工清洗。

## 性能与资源占用

**界面流畅度与 CPU**：

- 批量哈希的 worker 线程在 Windows 上以低于正常优先级运行，满载计算时不抢占界面与其他程序；
- 进度上报两级节流：引擎每 8 MiB 汇总一次块级进度，GUI 线程每 100 ms 最多接收一次快照，事件循环不会被大队列冲垮；
- 工具页懒加载：未打开过的工具不创建界面；打开过的页面为保留状态而常驻内存（至多 10 个轻量页面），隐藏时不参与渲染。

**磁盘体积**：

- 全部 QML 通过 `qt_add_qml_module` 编译进二进制（qmlcachegen），发布目录没有松散的 .qml/.qmlc 文件；
- 界面不使用图片与 QtSvg：图标为字符字形，不部署 imageformats 插件；
- 控件使用 Qt Quick Controls 的 Basic 样式（最小、跟随应用调色板），不引入 Material/Universal；
- 发布打包裁剪：不含 OpenGL 软件回退、系统 D3D 编译器、翻译文件，以及 QtNetwork 的 TLS/网络状态插件（纯离线应用用不到）；Release 构建启用链接时优化（IPO/LTO）进一步缩小二进制；
- 保存与输出写入均为原子替换，且不会覆盖队列中的输入文件。

## 构建

需要 CMake 3.21+ 与 Qt 6.5+（Core、Quick、QuickControls2、Test），Windows 上使用 MSVC：

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

构建产物为 `hash.exe`（CLI，控制台子系统）与 `hash-gui.exe`（GUI，windowsgui 子系统）。Qt 程序发布时需要随附 Qt 运行库：用 `windeployqt` 组装发布目录，CI 的 release workflow 会在打 `v*` tag 时自动完成并打包为 zip + SHA-256 校验文件。

**本地运行 GUI**：直接双击刚构建出的 `hash-gui.exe` 会因缺少 Qt 运行库（DLL 与 QML 模块插件）而无法启动，需要先组装运行时：

```text
windeployqt --release --qmldir src/app build\Release\hash-gui.exe
```

GUI 子系统没有控制台，QML 诊断会写入 `%LOCALAPPDATA%\Hash\hash-gui\gui.log`；启动失败（窗口未创建）时会记录明确的错误条目，可据此排查缺哪个模块。

架构与模块划分见 [docs/architecture.md](docs/architecture.md)。程序不上传文件内容，也不要求管理员权限。MD5、SHA-1、CRC、Adler-32 和 FNV 仅用于兼容性、错误检测或非加密场景，不提供现代碰撞安全性。
