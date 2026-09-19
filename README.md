# Hash Tool

一个面向 Windows 的批量文件哈希工具，基于 **C++17 与 Qt 6** 构建，提供 CLI 和 Qt Widgets GUI 两种入口，参考 `keir.net/hash.html` 的文件列表、批量计算、复制和文本导出能力，并扩展更多算法。

## 支持算法

- 密码学哈希：SHA-224、SHA-256、SHA-384、SHA-512、SHA-512/224、SHA-512/256
- 传统兼容：MD5、SHA-1（不适合安全用途）
- 校验和：Adler-32、CRC-32 IEEE/Castagnoli/Koopman、CRC-64 ECMA/ISO
- 非加密哈希：FNV-1/FNV-1a 的 32/64/128 位版本

标准摘要（MD5、SHA 系列）由 Qt 内置的 `QCryptographicHash` 计算；Adler-32、CRC、FNV 与 SHA-512/224、SHA-512/256 为项目内置实现，全部通过标准测试向量校验。

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

## GUI

运行 `hash-gui.exe` 后，可以点击“添加文件”或把多个文件拖入窗口。默认选中 SHA-256，可选择多个算法；计算过程中显示总进度和当前文件，完成后可以复制结果、保存为 UTF-8 文本，或把一份已有摘要清单粘贴进来逐项比对（匹配/不匹配/缺少/多余/重复计数）。保存与输出写入均为原子替换，且不会覆盖队列中的输入文件。

## 构建

需要 CMake 3.21+ 与 Qt 6.2+（Core、Widgets、Test），Windows 上使用 MSVC：

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

构建产物为 `hash.exe`（CLI，控制台子系统）与 `hash-gui.exe`（GUI，windowsgui 子系统）。Qt 程序发布时需要随附 Qt 运行库：用 `windeployqt` 组装发布目录，CI 的 release workflow 会自动完成并打包为 zip。

程序不上传文件内容，也不要求管理员权限。MD5、SHA-1、CRC、Adler-32 和 FNV 仅用于兼容性、错误检测或非加密场景，不提供现代碰撞安全性。
