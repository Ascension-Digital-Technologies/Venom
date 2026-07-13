param(
  [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')]
  [string]$Config = 'Release',
  [string]$BuildDir = 'build',
  [switch]$FullQuickJS,
  [switch]$Werror
)

$ErrorActionPreference = 'Stop'
$Root = Resolve-Path (Join-Path $PSScriptRoot '..')
$args = @('-ExecutionPolicy','Bypass','-File',(Join-Path $PSScriptRoot 'build.ps1'),'-Config',$Config,'-BuildDir',$BuildDir)
if ($FullQuickJS) { $args += '-FullQuickJS' }
if ($Werror) { $args += '-Werror' }
& powershell @args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[venom] test: $BuildDir ($Config)"
& ctest --test-dir (Join-Path $Root $BuildDir) --build-config $Config --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
