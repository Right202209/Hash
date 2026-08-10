$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$build = @{
    CGO_ENABLED = "0"
    GOOS        = "windows"
    GOARCH      = "amd64"
}

function Invoke-Checked {
    param([string]$Command, [scriptblock]$Action)
    & $Action
    if ($LASTEXITCODE -ne 0) {
        throw "$Command failed with exit code $LASTEXITCODE"
    }
}

Invoke-Checked "go test" { go test ./... }
Invoke-Checked "go test -race" { go test -race ./... }

$env:CGO_ENABLED = $build.CGO_ENABLED
$env:GOOS = $build.GOOS
$env:GOARCH = $build.GOARCH
Invoke-Checked "go build hash.exe" { go build -trimpath -ldflags "-s -w" -o "$root/hash.exe" ./cmd/hash }
Invoke-Checked "go build hash-gui.exe" { go build -trimpath -ldflags "-s -w -H=windowsgui" -o "$root/hash-gui.exe" ./cmd/hash-gui }
