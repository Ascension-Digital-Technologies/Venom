param(
  [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')][string]$Config = 'Release',
  [string]$BuildDir = 'build',
  [switch]$FullQuickJS,
  [switch]$Werror
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'internal/console.ps1')
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
try {
  Write-VenomBanner -Title 'Test suite' -Subtitle "$Config configuration"
  & (Join-Path $PSScriptRoot 'build.ps1') -Config $Config -BuildDir $BuildDir -FullQuickJS:$FullQuickJS -Werror:$Werror
  if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
  Invoke-VenomExternal -Program 'ctest' -Arguments @('--test-dir',(Join-Path $Root $BuildDir),'--build-config',$Config,'--output-on-failure') -Description 'Run CTest inventory'
  Write-VenomSuccess 'All tests passed.'
  exit 0
} catch {
  Write-VenomFailure $_.Exception.Message
  exit 1
}
