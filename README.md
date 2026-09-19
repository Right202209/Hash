# Hash Tool

一个面向 Windows 的文件工具箱，基于 **C++17 与 Qt 6** 构建，提供 CLI 和 Qt Quick (QML) GUI 两种入口。核心是参考 `keir.net/hash.html` 的批量文件哈希校验，并像 uTools 一样内置一组轻量工具（文本哈希、Base64、JSON、时间戳等）。

## 支持算法

- 密码学哈希：SHA-224、SHA-256、SHA-384、SHA-512、SHA-512/224、SHA-512/256
- 传统兼容：MD5、SHA-1（不适合安全用途）
- 校验和：Adler-32、CRC-32 IEEE/Castagnoli/Koopman、CRC-64 ECMA/ISO
- 非加密哈希：FNV-1/FNV-1a 的 32/64/128 位版本

标准摘要（MD5、SHA 系列）由 Qt 内置的 `QCryptographicHash` 计算；Adler-32、CRC、FNV 与 SHA-512/224、SHA-512/256 为项目内置实现，全部通过标准测试向量校验。文件哈希与文本哈希共用同一套 digester 实现。

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

选项包括 `--algorithm/-a`、`--all`、`--size`、`--modified`、`--utc`、`--workers`、`--format text|tsv|json`、`--output/-o`、`--recursive`、`--literal` 和 `--fail-fast`。Windows 下通配符由程序展开；默认按输入顺序输出，文件读取采用流式处理，不会把整个文件载入内存。退出码：0 成功，1 失败，2 用法错误，3 已取消。

## GUI（工具箱）

`hash-gui.exe` 是一个启动器式工具箱：顶部命令栏输入即筛选工具，回车打开第一个匹配项，Esc 从工具页返回主页；主页为工具卡片网格，工具页在进入时才创建（懒加载）。

内置工具：

- **文件哈希** — 批量计算文件摘要：拖拽或选择文件，多算法单次读取并行计算，实时进度与取消；支持复制/保存结果（原子写入）、把一份已有摘要清单粘贴进来逐项比对（匹配/不匹配/缺少/多余/重复计数）
- **文本哈希** — 计算一段文本的任意注册摘要（UTF-8）
- **Base64 编解码** / **URL 编解码** — 编码、解码与输入输出互换，解码容错空白
- **JSON 格式化** — 格式化、压缩与校验，错误定位到行
- **时间戳** — Unix 秒/毫秒与日期时间互换，本地时区与 UTC 双显示
- **UUID 生成** — 批量生成 UUID v4，可控制大小写、大括号与连字符
- **进制转换** — 2-36 进制整数互换，支持负数
- **颜色转换** — HEX/RGB/HSL 互换，支持 Alpha 通道
- **密码生成** — 按长度与字符集批量生成随机密码（系统安全随机源），可排除易混淆字符

保存与输出写入均为原子替换，且不会覆盖队列中的输入文件。

## 资源占用

- 全部 QML 通过 `qt_add_qml_module` 编译进二进制（qmlcachegen），发布目录中没有松散的 .qml/.qmlc 文件
- 界面不使用图片与 QtSvg：图标为字符字形，不部署 imageformats 插件
- 控件使用 Qt Quick Controls 的 Basic 样式（最小、跟随应用调色板），不引入 Material/Universal
- 工具页懒加载，同一时刻只有当前工具的界面在内存中
- 发布打包裁剪：不含 OpenGL 软件回退（Qt 6 在 Windows 默认走 D3D11）、系统 D3D 编译器与翻译文件（见 release workflow 的 `windeployqt` 参数）

## 构建

需要 CMake 3.21+ 与 Qt 6.5+（Core、Quick、QuickControls2、Test），Windows 上使用 MSVC：

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

构建产物为 `hash.exe`（CLI，控制台子系统）与 `hash-gui.exe`（GUI，windowsgui 子系统）。Qt 程序发布时需要随附 Qt 运行库：用 `windeployqt` 组装发布目录，CI 的 release workflow 会自动完成并打包为 zip。

**本地运行 GUI**：直接双击刚构建出的 `hash-gui.exe` 会因缺少 Qt 运行库（DLL 与 QML 模块插件）而无法启动，需要先组装运行时：

```text
windeployqt --release --qmldir src/app build\Release\hash-gui.exe
```

GUI 子系统没有控制台，QML 诊断会写入 `%LOCALAPPDATA%\Hash\hash-gui\gui.log`；启动失败（窗口未创建）时会记录明确的错误条目，可据此排查缺哪个模块。

程序不上传文件内容，也不要求管理员权限。MD5、SHA-1、CRC、Adler-32 和 FNV 仅用于兼容性、错误检测或非加密场景，不提供现代碰撞安全性。
