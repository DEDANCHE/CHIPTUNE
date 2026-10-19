<#
  Builds unit/ + src/ into chipapu.nts1mkiiunit with the logue SDK (NTS-1 mkII) makefiles.
  Works out of build/unit, so the SDK checkout is left untouched.

  Needs: logue-sdk checkout ($env:LOGUE_SDK or -Sdk), arm-none-eabi-gcc (PATH or -ToolchainBin),
         GNU make plus sh/mkdir/cp/rm (Git for Windows' usr\bin is added if sh is not on PATH), git.
  Usage: powershell -File tools\build_unit.ps1 [-Sdk C:\path\logue-sdk] [-ToolchainBin <dir>]
#>
param(
  [string]$Sdk = $env:LOGUE_SDK,
  [string]$ToolchainBin,
  [string]$Project = "chipapu"
)
$ErrorActionPreference = "Stop"
$repo = Split-Path $PSScriptRoot
if (-not $Sdk) { throw "Set -Sdk or LOGUE_SDK to the logue-sdk checkout." }
$plat = Join-Path $Sdk "platform\nts-1_mkii"
if (-not (Test-Path "$plat\dummy-osc\unit.cc")) { throw "Not a logue-sdk checkout: $Sdk" }

if (-not $ToolchainBin) {
  $gcc = Get-Command arm-none-eabi-gcc -ErrorAction SilentlyContinue
  if (-not $gcc) { throw "arm-none-eabi-gcc not found; pass -ToolchainBin." }
  $ToolchainBin = Split-Path $gcc.Source
}
# The SDK makefile does not quote the compiler path, so use the 8.3 form when it has spaces.
$short = cmd /c "for %I in (`"$ToolchainBin`") do @echo %~sI"
$bin = ($short.Trim()) -replace '\\', '/'

if (-not (Get-Command sh.exe -ErrorAction SilentlyContinue)) {
  $gitRoot = Split-Path (Split-Path (Split-Path (git --exec-path)))
  $env:PATH = "$gitRoot\usr\bin;$env:PATH"
}

$cmsis = Join-Path $repo "build\CMSIS"
if (-not (Test-Path "$cmsis\CMSIS\Include\arm_math.h")) {
  git clone --depth 1 https://github.com/ARM-software/CMSIS.git $cmsis
}

$proj = Join-Path $repo "build\unit"
New-Item -ItemType Directory -Force $proj | Out-Null
Copy-Item "$plat\dummy-osc\unit.cc", "$plat\dummy-osc\Makefile" $proj -Force
(Get-Content "$plat\dummy-osc\config.mk" -Raw).Replace("PROJECT := dummy_osc", "PROJECT := $Project") |
  Set-Content "$proj\config.mk" -NoNewline
Copy-Item "$repo\unit\header.c", "$repo\unit\osc.h", "$repo\src\apu_core.h" $proj -Force

function Fwd($p) { $p -replace '\\', '/' }
Push-Location $proj
try {
  make install -j4 `
    PROJECT_ROOT="$(Fwd $proj)/" `
    COMMON_INC_PATH="$(Fwd "$plat\common")" COMMON_SRC_PATH="$(Fwd "$plat\common")" `
    LDDIR="$(Fwd "$plat\ld")" CMSISDIR="$(Fwd "$cmsis\CMSIS")" GCC_BIN_PATH="$bin"
  if ($LASTEXITCODE) { throw "make failed ($LASTEXITCODE)" }
} finally { Pop-Location }
Get-Item "$proj\$Project.nts1mkiiunit" | Select-Object FullName, Length
