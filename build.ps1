[CmdletBinding()]
param(
    [switch]$Test,
    [switch]$Clean,
    [string]$MinGWPath = "D:\EDCs\Coding\MinGW\bin"
)

$ErrorActionPreference = "Stop"

if (Test-Path $MinGWPath) {
    $env:Path = "$MinGWPath;" + $env:Path
}

if ($Clean) {
    Write-Host "[Clean] Removing build artifacts..." -ForegroundColor Yellow
    Get-ChildItem -Path "src", "res", "tests" -Filter "*.o" -Recurse | Remove-Item -Force -ErrorAction SilentlyContinue
    Remove-Item -Path "FanControlHotkey.exe", "tests/test_main.exe", "res/resource.o" -Force -ErrorAction SilentlyContinue
    Write-Host "[Clean] Done." -ForegroundColor Green
    if (-not $Test) { return }
}

$srcFiles = @(
    "src/main.c",
    "src/strings.c",
    "src/config.c",
    "src/hotkey.c",
    "src/process_monitor.c",
    "src/runner.c",
    "src/dpi_utils.c",
    "src/ui_main.c",
    "src/ui_settings.c"
)

$testSrcFiles = @(
    "tests/test_main.c",
    "src/strings.c",
    "src/config.c",
    "src/hotkey.c",
    "src/process_monitor.c",
    "src/runner.c",
    "src/dpi_utils.c"
)

if ($Test) {
    Write-Host "[Build] Compiling test suite..." -ForegroundColor Cyan
    & gcc -O2 -Wall -Wextra -D_UNICODE -DUNICODE -municode $testSrcFiles -o tests/test_main.exe -ladvapi32 -luser32 -lgdi32 -lshell32
    if ($LASTEXITCODE -ne 0) { throw "Test compilation failed!" }
    Write-Host "[Test] Running tests..." -ForegroundColor Cyan
    & .\tests\test_main.exe
    if ($LASTEXITCODE -ne 0) { throw "Tests failed!" }
    Write-Host "[Test] Success." -ForegroundColor Green
    return
}

Write-Host "[Build] Compiling Windows resource..." -ForegroundColor Cyan
& windres res/resource.rc -O coff -o res/resource.o
if ($LASTEXITCODE -ne 0) { throw "windres failed!" }

Write-Host "[Build] Compiling FanControlHotkey.exe..." -ForegroundColor Cyan
& gcc -O2 -Wall -Wextra -D_UNICODE -DUNICODE -municode -mwindows $srcFiles res/resource.o -o FanControlHotkey.exe -luser32 -lshell32 -lcomdlg32 -ladvapi32 -lgdi32
if ($LASTEXITCODE -ne 0) { throw "Build failed!" }

Write-Host "[Build] Success: FanControlHotkey.exe generated successfully." -ForegroundColor Green
