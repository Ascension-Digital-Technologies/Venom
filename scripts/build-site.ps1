param(
  [string]$Site = "",
  [string]$Dist = "dist",
  [string]$Config = "Release",
  [switch]$RebuildRuntime
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
if ([string]::IsNullOrWhiteSpace($Site)) { $Site = Join-Path $root 'examples/basic-site' }
if ($RebuildRuntime) { & (Join-Path $PSScriptRoot 'build-quickjs-wasm.ps1') -Force; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }
$distPath = if ([System.IO.Path]::IsPathRooted($Dist)) { $Dist } else { Join-Path $root $Dist }
& (Join-Path $PSScriptRoot 'build.ps1') -Config $Config -BuildDir 'build/windows-release' -FullQuickJS
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$candidates = @(
  (Join-Path $root 'build/windows-release/venom.exe'),
  (Join-Path $root "build/windows-release/$Config/venom.exe"),
  (Join-Path $root 'build/windows-release/venom'),
  (Join-Path $root "build/windows-release/$Config/venom")
)
$venom = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $venom) { throw 'venom executable not found under build/windows-release/' }
Write-Host "[venom] production build $Site => $distPath"
& $venom build $Site --out $distPath
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $venom verify-runtime $distPath --target browser --require-real-engine
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$pkg = Get-ChildItem -Path (Join-Path $distPath 'assets/app') -Filter 'app.*.vbc' | Select-Object -First 1
if (!$pkg) { throw "package was not emitted under $distPath/assets/app" }
Write-Host "[venom] package: $($pkg.FullName)"
