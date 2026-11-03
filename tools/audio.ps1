param([ValidateSet('audition','profile')][string]$Mode='audition',[string[]]$Presets=@(),[string]$OutputDirectory='build/audition')
$ErrorActionPreference='Stop'
Push-Location (Split-Path $PSScriptRoot)
try {
  New-Item -ItemType Directory -Force build/host,$OutputDirectory | Out-Null
  zig c++ -std=c++11 -Wall -Wextra -O2 tools/audio_tools.cpp -o build/host/audio_tools.exe
  if($LASTEXITCODE){throw 'Audio tool compile failed'}
  & ./build/host/audio_tools.exe $Mode $OutputDirectory @Presets
  if($LASTEXITCODE){throw 'Audio tool failed'}
} finally { Pop-Location }
