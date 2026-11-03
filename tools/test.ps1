param([switch]$NoSanitize)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot
Push-Location $repo
try {
  New-Item -ItemType Directory -Force build/host,build/audio | Out-Null
  $flags=@('-std=c++11','-Wall','-Wextra','-O2','-Itests/sdk_stubs','-Isrc')
  if (-not $NoSanitize) { $flags+=@('-fsanitize=undefined','-fno-sanitize-recover=all') }
  zig cc -std=c11 -Itests/sdk_stubs -c platform/nts1_mkii/header.c -o build/host/header.o
  if ($LASTEXITCODE) { throw 'Header compile failed' }
  $total=0
  foreach ($suite in @('adapter_test','features_test','runtime_test','core_audio_test')) {
    $sources=@("tests/$suite.cpp")
    if ($suite -eq 'runtime_test') { $sources+='build/host/header.o' }
    zig c++ @flags @sources -o "build/host/$suite.exe"
    if ($LASTEXITCODE) { throw "$suite compile failed" }
    $arguments=@();if($suite -eq 'core_audio_test'){$arguments+='build/audio'}
    $output=& "./build/host/$suite.exe" @arguments
    $code=$LASTEXITCODE
    $output | Set-Content "build/host/$suite-results.txt"
    $output | Select-String 'FAIL'
    $total+=$output.Count
    Write-Output "$suite : $($output.Count) checks; exit $code"
    if($code){throw "$suite failed"}
  }
  Write-Output "Total: $total checks passed"
} finally { Pop-Location }
