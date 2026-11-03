<#
  Builds platform/nts1_mkii/ + src/ into chipapu.nts1mkiiunit with the logue SDK (NTS-1 mkII) makefiles.
  Works out of build/unit, so the SDK checkout is left untouched.

  Needs: logue-sdk checkout ($env:LOGUE_SDK or -Sdk), arm-none-eabi-gcc (PATH or -ToolchainBin),
         GNU make plus sh/mkdir/cp/rm (Git for Windows' usr\bin is added if sh is not on PATH), git.
  Usage: powershell -File tools\build_unit.ps1 [-Sdk C:\path\logue-sdk] [-ToolchainBin <dir>]
#>
param(
  [string]$Sdk = $env:LOGUE_SDK,
  [string]$ToolchainBin,
  [string]$Project = "chipapu",
  [switch]$Profile
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

$cmsis = Join-Path $Sdk "platform\ext\CMSIS"
$cmsisRevision = 'f25cf2c15304ad872a5b9761651d3f57d6202906'
if (-not (Test-Path "$cmsis\CMSIS\Include\arm_math.h")) {
  $cmsis = Join-Path $repo "build\CMSIS"
}
if (-not (Test-Path "$cmsis\CMSIS\Include\arm_math.h")) {
  git init $cmsis
  if ($LASTEXITCODE) { throw 'CMSIS initialization failed' }
  git -C $cmsis fetch --depth 1 https://github.com/ARM-software/CMSIS.git $cmsisRevision
  if ($LASTEXITCODE) { throw 'Pinned CMSIS fetch failed' }
  git -C $cmsis checkout --detach FETCH_HEAD
  if ($LASTEXITCODE) { throw 'Pinned CMSIS checkout failed' }
}
$actualCmsisRevision = git -C $cmsis rev-parse HEAD
if ($LASTEXITCODE -or $actualCmsisRevision.Trim() -ne $cmsisRevision) { throw "CMSIS revision must be $cmsisRevision" }

$proj = Join-Path $repo "build\unit"
New-Item -ItemType Directory -Force $proj | Out-Null
Copy-Item "$repo\platform\nts1_mkii\unit.cc", "$plat\dummy-osc\Makefile" $proj -Force
(Get-Content "$plat\dummy-osc\config.mk" -Raw).Replace("PROJECT := dummy_osc", "PROJECT := $Project") |
  Set-Content "$proj\config.mk" -NoNewline
if ($Profile) { Add-Content "$proj\config.mk" "`nUDEFS += -DCHIPAPU_PROFILE" }
# The SDK makefile has no header dependencies, so always rebuild from scratch.
$buildDir = [IO.Path]::GetFullPath((Join-Path $proj "build"))
$projectDir = [IO.Path]::GetFullPath($proj)
if ((Split-Path $buildDir -Parent) -ne $projectDir) { throw "Build directory is outside the staged project." }
if (Test-Path -LiteralPath $buildDir) { Remove-Item -LiteralPath $buildDir -Recurse -Force }
Get-ChildItem -LiteralPath $projectDir -Filter "*.nts1mkiiunit" -File |
  ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force }
Copy-Item "$repo\platform\nts1_mkii\profiling.h", "$repo\platform\nts1_mkii\parameters.h", "$repo\platform\nts1_mkii\header.c", "$repo\platform\nts1_mkii\osc.h", "$repo\src\apu_core.h", "$repo\src\presets.h" $proj -Force

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
