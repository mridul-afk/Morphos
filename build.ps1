
<#
.SYNOPSIS
    MORPHOS C++17 build script.

.EXAMPLE
    .\build.ps1

.EXAMPLE
    .\build.ps1 -Test

.EXAMPLE
    .\build.ps1 -Run

.EXAMPLE
    .\build.ps1 -Clean -Test

.EXAMPLE
    .\build.ps1 -Debug -Test
#>

[CmdletBinding()]
param(
  [switch]$Test,
  [switch]$Run,
  [switch]$Clean,
  [switch]$DebugBuild,
  [switch]$Help,

  [ValidateRange(1, 128)]
  [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

# Resolve project paths independently of current directory.
$RootDir = $PSScriptRoot
$BuildDir = Join-Path $RootDir "build"

$Config = if ($DebugBuild) { "Debug" } else { "Release" }
$Generator = "Visual Studio 17 2022"
$Architecture = "x64"

function Invoke-Checked {
  param(
    [string]$Description,
    [string]$Executable,
    [string[]]$Arguments
  )

  Write-Host ""
  Write-Host "==> $Description" -ForegroundColor Cyan

  & $Executable @Arguments

  if ($LASTEXITCODE -ne 0) {
    throw "$Description failed with exit code $LASTEXITCODE"
  }
}

if ($Help) {
  Write-Host @"

MORPHOS Build System

Usage:
    .\build.ps1 [options]

Options:
    -Test       Build and run GoogleTest suite
    -Run        Build and run MORPHOS simulation
    -Clean      Remove build directory before building
    -Debug      Use Debug instead of Release
    -Jobs N     Parallel build jobs (default: CMake/MSBuild)
    -Help       Display this help

Examples:
    .\build.ps1
    .\build.ps1 -Test
    .\build.ps1 -Run
    .\build.ps1 -Clean -Test
    .\build.ps1 -Debug -Test
    .\build.ps1 -Jobs 4

"@
  exit 0
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Magenta
Write-Host "             MORPHOS BUILD              " -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor Magenta

Write-Host "Project : $RootDir"
Write-Host "Config  : $Config"
Write-Host "Tests   : $Test"
Write-Host "Run     : $Run"
Write-Host ""

# Verify required tools.
foreach ($Tool in @("cmake")) {
  if (-not (Get-Command $Tool -ErrorAction SilentlyContinue)) {
    throw "Required tool '$Tool' was not found in PATH."
  }
}

# Clean previous build when requested.
if ($Clean -and (Test-Path $BuildDir)) {
  Write-Host "==> Removing existing build directory..." -ForegroundColor Yellow
  Remove-Item -LiteralPath $BuildDir -Recurse -Force
}

# Configure.
# BUILD_TESTING controls whether GoogleTest is built.
$BuildTesting = if ($Test) { "ON" } else { "OFF" }

# When running an existing build configured with tests,
# switching back to a normal build disables them.
Invoke-Checked `
  -Description "Configuring MORPHOS" `
  -Executable "cmake" `
  -Arguments @(
  "-S", $RootDir,
  "-B", $BuildDir,
  "-G", $Generator,
  "-A", $Architecture,
  "-DBUILD_TESTING=$BuildTesting"
)

# Build.
$BuildArgs = @(
  "--build", $BuildDir,
  "--config", $Config
)

if ($Jobs -gt 0) {
  $BuildArgs += @("--parallel", "$Jobs")
}

Invoke-Checked `
  -Description "Building MORPHOS ($Config)" `
  -Executable "cmake" `
  -Arguments $BuildArgs

# Run tests explicitly.
if ($Test) {
  Invoke-Checked `
    -Description "Running GoogleTest suite" `
    -Executable "ctest" `
    -Arguments @(
    "--test-dir", $BuildDir,
    "-C", $Config,
    "--output-on-failure"
  )
}

# Run simulator explicitly.
if ($Run) {
  $ExePath = Join-Path $BuildDir "$Config\morphos.exe"

  if (-not (Test-Path $ExePath)) {
    throw "MORPHOS executable not found: $ExePath"
  }

  Invoke-Checked `
    -Description "Running MORPHOS simulation" `
    -Executable $ExePath `
    -Arguments @()
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "       MORPHOS BUILD SUCCESSFUL          " -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "Configuration: $Config"
Write-Host "Build path   : $BuildDir"
