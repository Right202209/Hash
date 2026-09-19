# Repository Guidelines

## Project Structure & Module Organization

`hash` is a C++17 + Qt 6 project providing a Windows-focused file toolbox (batch file hashing plus uTools-style utilities) with CLI and GUI entry points.

- `src/core/` — streaming hash engine: digester implementations (Qt `QCryptographicHash` plus built-in Adler-32, CRC-32/64, FNV, SHA-512/224, SHA-512/256), the algorithm registry, single-file hashing and batch orchestration.
- `src/common/` — shared utilities: atomic output-file replacement with input-file protection, platform-aware path keys, digest-listing parsing and comparison, Go-style string quoting helpers, filesystem path conversions.
- `src/cli/` — command-line entry point, argument parsing, path expansion, and text/TSV/JSON output formatting.
- `src/tools/` — toolbox backends in a QtCore-only static library `hash_tools`: tool-registry, algorithm-registry and file-queue models, the threaded `HashController` for batch hashing from the GUI, and one small QObject controller per tool (text digest, Base64/URL codec, JSON, timestamp, UUID, radix, color, password).
- `src/app/` — Qt Quick (QML) GUI: a launcher-style shell (command bar, tool grid, lazy-loaded tool pages), QML singletons for theme and notifications, plus `main.cpp` (dark application palette, singleton registration) and the clipboard service.
- `tests/` — Qt Test suites in `test_*.h`, driven by `tests/main.cpp`.
- `CMakeLists.txt` — static libraries `hash_core` and `hash_tools` plus the `hash` (console) and `hash_tests` targets; `hash-gui` (windowsgui, QML compiled in via `qt_add_qml_module`) is defined in `src/app/CMakeLists.txt`, beside the QML files the module registers.

## Build, Test, and Development Commands

Agents must not run builds or tests locally; GitHub Actions is the only place builds and tests execute. The commands below document what CI runs.

CI workflow: `.github/workflows/ci.yml` (clang-format check plus Qt builds and tests on Ubuntu and Windows; the Windows job produces the release binaries).

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure --no-tests=error
clang-format --dry-run -Werror $(git ls-files '*.cpp' '*.h')
```

Release workflow: `.github/workflows/release.yml` builds on Windows on `v*` tags, assembles `hash.exe`, `hash-gui.exe` and the Qt runtime with `windeployqt`, and publishes a zip plus a SHA-256 checksum file on the GitHub release.

## Coding Style & Naming Conventions

Formatting is enforced by `.clang-format` (LLVM base, 4-space indent, 100-column limit); run clang-format before submitting. Types use `PascalCase`, functions and methods use `camelCase`, member variables carry the `m` prefix (for example `mQueue`), and constants use `kPascalCase`. Prefer Qt containers in interface signatures. Errors are returned as empty-or-populated `QString` messages (or boolean returns with an out-param); do not use exceptions for control flow. Keep modules small and single-purpose under `src/`. Algorithm names are lowercase registry keys (for example `sha256`, `crc32-ieee`). Source files are UTF-8; MSVC builds pass `/utf-8`.

## Testing Guidelines

Use the Qt Test framework: one `QObject` test class per `tests/test_*.h` header with private slots named after the behavior (for example `oneReadComputesEveryRequestedAlgorithm`), registered in `tests/main.cpp`. Use `QTemporaryDir` for filesystem fixtures, `QSKIP` when a platform feature (hard links, symlinks) is unavailable, and guard platform-dependent expectations with `#ifdef Q_OS_WIN`. Add or update tests as needed, but never execute them locally.

## Commit & Pull Request Guidelines

Commits use short, imperative, capitalized subjects (for example `Rewrite the hashing tool in C++ with Qt 6`). Keep each commit focused. Pull requests should describe the change, note affected modules, link related issues, and include screenshots for GUI changes. Rely on GitHub Actions for build and test results and note any Windows-only behavior that could not be verified.

## Agent-Specific Instructions

- Modify project source, tests, and documentation only; never run the project, its binaries, its build, or its test suite.
- All build and test execution happens in GitHub Actions. Rely on CI results instead of local runs.
- Do not add local build or test steps to your workflow. If verification is needed, update or extend the CI workflows and let them run.
- When a change cannot be validated without running code, state that clearly in the pull request instead of running it.
