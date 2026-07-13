param(
  [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')]
  [string]$Config = 'Release',
  [string]$BuildDir = 'build',
  [switch]$FullQuickJS,
  [switch]$Werror
)

$ErrorActionPreference = 'Stop'
$Root = Resolve-Path (Join-Path $PSScriptRoot '..')
$BuildPath = Join-Path $Root $BuildDir

$configure = @('-S', $Root, '-B', $BuildPath, "-DCMAKE_BUILD_TYPE=$Config")
if ($FullQuickJS) { $configure += '-DVENOM_ENABLE_FULL_QUICKJS=ON' }
if ($Werror) { $configure += '-DVENOM_ENABLE_WERROR=ON' }

Write-Host "[venom] configure: $BuildDir ($Config)"
& cmake @configure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[venom] build: $BuildDir ($Config)"
& cmake --build $BuildPath --config $Config --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$candidates = @(
  (Join-Path $BuildPath 'venom.exe'),
  (Join-Path $BuildPath "$Config/venom.exe"),
  (Join-Path $BuildPath 'Debug/venom.exe'),
  (Join-Path $BuildPath 'Release/venom.exe'),
  (Join-Path $BuildPath 'RelWithDebInfo/venom.exe'),
  (Join-Path $BuildPath 'MinSizeRel/venom.exe')
)
$venom = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $venom) { throw "venom.exe was not found under $BuildPath" }
Write-Host "[venom] built: $venom"
