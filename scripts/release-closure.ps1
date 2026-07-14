param(
  [string]$BuildDir = 'build/release-closure',
  [string]$OutDir = 'build/release-closure-output',
  [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')][string]$Config = 'Release',
  [int]$Parallel = 0,
  [switch]$NoClean,
  [switch]$NoHardener,
  [switch]$NoTests,
  [switch]$NoExamples,
  [switch]$NoPackage,
  [switch]$NoWerror,
  [switch]$BrowserRuntimeTests,
  [switch]$KeepGoing
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'internal/console.ps1')
Write-VenomBanner -Title 'Release closure' -Subtitle "$Config configuration"
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Python = Get-Command python -ErrorAction SilentlyContinue
if (-not $Python) { $Python = Get-Command py -ErrorAction SilentlyContinue }
if (-not $Python) { throw 'Python 3.10 or newer is required.' }
$argsList = @((Join-Path $Root 'tools/run_release_closure.py'), '--repo-root', $Root, '--build-dir', $BuildDir, '--out-dir', $OutDir, '--config', $Config)
if ($Parallel -gt 0) { $argsList += @('--parallel', [string]$Parallel) }
if ($NoClean) { $argsList += '--no-clean' }
if ($NoHardener) { $argsList += '--no-install-hardener' }
if ($NoTests) { $argsList += '--no-run-tests' }
if ($NoExamples) { $argsList += '--no-build-examples' }
if ($NoPackage) { $argsList += '--no-package' }
if ($NoWerror) { $argsList += '--no-werror' }
if ($BrowserRuntimeTests) { $argsList += '--browser-runtime-tests' }
if ($KeepGoing) { $argsList += '--keep-going' }
if ($Python.Name -match '^py(?:\.exe)?$') { & $Python.Source -3 @argsList } else { & $Python.Source @argsList }
$code=$LASTEXITCODE
if($code -eq 0){ Write-VenomSuccess 'Release closure completed successfully.' } else { Write-VenomFailure "Release closure failed with exit code $code." }
exit $code
