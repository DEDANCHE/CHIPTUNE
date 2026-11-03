param([string]$Sdk=$env:LOGUE_SDK,[string]$ToolchainBin)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot
Push-Location $repo
try {
  & "$PSScriptRoot/test.ps1"
  & "$PSScriptRoot/build_unit.ps1" -Sdk $Sdk -ToolchainBin $ToolchainBin
  if (Get-Command node -ErrorAction SilentlyContinue) {
    node tools/check_unit.cjs build/unit/chipapu.nts1mkiiunit build/unit/manifest.json
  } else {
    python tools/check_unit.py build/unit/chipapu.nts1mkiiunit build/unit/manifest.json
  }
  if($LASTEXITCODE){throw 'Firmware validation failed; dist was not updated'}
  Copy-Item -LiteralPath build/unit/chipapu.nts1mkiiunit -Destination dist/chipapu.nts1mkiiunit -Force
  Copy-Item -LiteralPath build/unit/manifest.json -Destination dist/manifest.json -Force
  $manifest=Get-Content dist/manifest.json -Raw | ConvertFrom-Json
  $manifest.package='chipapu.nts1mkiiunit'
  $manifest | Add-Member version '2.1.0'
  $manifest | Add-Member cmsisRevision 'f25cf2c15304ad872a5b9761651d3f57d6202906'
  $compiler = if ($ToolchainBin) { Join-Path $ToolchainBin 'arm-none-eabi-gcc.exe' } else { (Get-Command arm-none-eabi-gcc).Source }
  $compilerVersion = & $compiler --version
  if ($LASTEXITCODE) { throw 'Cannot record compiler version' }
  $manifest | Add-Member compiler ($compilerVersion | Select-Object -First 1)
  $manifest | ConvertTo-Json | Set-Content dist/manifest.json
  Write-Output "Release ready: dist/chipapu.nts1mkiiunit ($($manifest.sha256))"
} finally { Pop-Location }
