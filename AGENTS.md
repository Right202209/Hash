# Repository Guidelines

## Project Structure & Module Organization

`hash` is a Go 1.23 module providing a Windows-focused batch file hashing tool with CLI and native GUI entry points.

- `cmd/hash/` — CLI entry point (`main.go`).
- `cmd/hash-gui/` — GUI entry point, Windows-only (`main_windows.go`).
- `internal/engine/` — streaming hash computation, batch orchestration, and shared types.
- `internal/registry/` — algorithm registry and constructor wiring.
- `internal/atomicfile/` — atomic output-file replacement with input-file protection.
- `internal/pathutil/` — platform-aware path normalization shared by the CLI and the comparison code.
- `internal/cli/` — argument parsing, path expansion, and output formatting.
- `internal/compare/` — parsing and comparison of expected versus actual digests.
- `internal/gui/` — Windows GUI window management, layout, theme, and progress handling.
- `scripts/build.ps1` — PowerShell build/test script.

Tests live beside the code as `*_test.go`. Windows-specific files use the `_windows.go` suffix plus `//go:build windows`; non-Windows fallbacks use `_nonwindows.go`.

## Build, Test, and Development Commands

Agents must not run these locally; GitHub Actions is the only place tests and builds execute. The commands below document what CI runs.

CI workflow: `.github/workflows/ci.yml` (format check, vet, tests, race tests, and Windows builds).

```text
go test ./...                 # run the full test suite
go test -race ./...           # run tests with the race detector
go vet ./...                  # static checks
gofmt -l .                    # list unformatted files
GOOS=windows GOARCH=amd64 CGO_ENABLED=0 go build -trimpath -ldflags "-s -w" -o hash.exe ./cmd/hash
GOOS=windows GOARCH=amd64 CGO_ENABLED=0 go build -trimpath -ldflags "-s -w -H=windowsgui" -o hash-gui.exe ./cmd/hash-gui
pwsh ./scripts/build.ps1      # test, race-test, and build both binaries
```

## Coding Style & Naming Conventions

Follow standard Go style: tabs for indentation, `gofmt`-clean code, and `go vet` passing. Exported identifiers use `CamelCase`; unexported helpers use `camelCase`. Keep packages small and single-purpose under `internal/`. Wrap errors with context using `fmt.Errorf("...: %w", err)`. Algorithm names are lowercase registry keys (for example `sha256`, `crc32-ieee`).

## Testing Guidelines

Use the standard `testing` package with table-driven tests where practical. Test functions are named `TestXxx_Behavior` (for example `TestHashFile_OneReadComputesEveryRequestedAlgorithm`). Prefer `t.TempDir()` for filesystem fixtures and call `t.Parallel()` where safe. Add or update tests as needed, but never execute them locally.

## Commit & Pull Request Guidelines

Commits use short, imperative, capitalized subjects (for example `Add Windows-specific GUI layout, theme, and progress handling`). Keep each commit focused. Pull requests should describe the change, note affected packages, link related issues, and include screenshots for GUI changes. Rely on GitHub Actions for test results and note any Windows-only behavior that could not be verified locally.

## Agent-Specific Instructions

- Modify project source, tests, and documentation only; never run the project, its binaries, or its test suite.
- All test and build execution happens in GitHub Actions. Rely on CI results instead of local runs.
- Do not add local test or build steps to your workflow. If verification is needed, update or extend the CI workflow and let it run.
- When a change cannot be validated without running code, state that clearly in the pull request instead of running it.
