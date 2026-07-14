[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$Example,
  [string]$DistName,
  [ValidateSet('dev','prod')][string]$Profile = 'prod',
  [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')][string]$Config = 'Release',
  [string]$BuildDir = 'build',
  [int]$Port = 8080,
  [switch]$NoOpen
)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Site = Join-Path $Root (Join-Path 'examples' $Example)
if (-not (Test-Path (Join-Path $Site 'index.html'))) { throw "Example not found: $Site" }
if ([string]::IsNullOrWhiteSpace($DistName)) { $DistName = "dist-$Example-$Profile" }
$Dist = Join-Path $Root $DistName

Write-Host "[venom] building $Example ($Profile)"
& (Join-Path $PSScriptRoot 'build-site.ps1') -Site $Site -Dist $Dist -BuildDir $BuildDir -Config $Config -Profile $Profile
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$Url = "http://127.0.0.1:$Port/"
Write-Host "[venom] serving $Example at $Url"
if (-not $NoOpen) {
  try { Start-Process $Url | Out-Null } catch { Write-Warning "Could not open the browser automatically: $($_.Exception.Message)" }
}
& python -m http.server $Port --bind 127.0.0.1 --directory $Dist
exit $LASTEXITCODE
