$ErrorActionPreference = 'Stop'
$Root = Resolve-Path (Join-Path $PSScriptRoot '..')
$dirs = @(
  'build','build-debug','build-release','build-fullqjs','build-werror',
  'dist','dist-basic','dist-basic-wasm','dist-basic-protect','dist-basic-release',
  'dist-release','dist-protect','dist-wasm'
)
foreach ($dir in $dirs) {
  $path = Join-Path $Root $dir
  if (Test-Path $path) {
    Write-Host "[venom] remove $dir"
    Remove-Item -Recurse -Force $path
  }
}
