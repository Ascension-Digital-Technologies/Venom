param([int]$Port = 8080, [string]$Dist = "dist")
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$distPath = if ([System.IO.Path]::IsPathRooted($Dist)) { $Dist } else { Join-Path $root $Dist }
if (!(Test-Path (Join-Path $distPath 'index.html'))) {
  & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'build-site.ps1') -Site (Join-Path $root 'examples/basic-site') -Dist $distPath
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
Write-Host "[venom] serving $distPath at http://127.0.0.1:$Port"
python -m http.server $Port --directory $distPath
