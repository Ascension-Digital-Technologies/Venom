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
. (Join-Path $PSScriptRoot 'internal/console.ps1')
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

try {
  $Site = Join-Path $Root (Join-Path 'examples' $Example)
  if (-not (Test-Path (Join-Path $Site 'index.html'))) { throw "Example not found: $Site" }
  if ([string]::IsNullOrWhiteSpace($DistName)) { $DistName = "dist-$Example-$Profile" }
  $Dist = Join-Path $Root $DistName
  $Url = "http://127.0.0.1:$Port/"

  Write-VenomBanner -Title 'Example launcher' -Subtitle $Example
  Write-VenomInfo "Profile: $Profile"
  Write-VenomInfo "URL:     $Url"

  & (Join-Path $PSScriptRoot 'build-site.ps1') -Site $Site -Dist $Dist -BuildDir $BuildDir -Config $Config -Profile $Profile
  if ($LASTEXITCODE -ne 0) { throw "Example build failed with exit code $LASTEXITCODE" }

  if (-not $NoOpen) {
    try { Start-Process $Url | Out-Null } catch { Write-VenomWarning "Browser could not be opened automatically: $($_.Exception.Message)" }
  }
  Write-VenomSuccess "Serving $Example at $Url"
  Write-VenomInfo 'Press Ctrl+C to stop the server.'
  & python -m http.server $Port --bind 127.0.0.1 --directory $Dist
  $code=$LASTEXITCODE
  if($code -ne 0){throw "HTTP server stopped with exit code $code"}
  Write-VenomSuccess 'Server stopped normally.'
  exit 0
} catch {
  Write-VenomFailure $_.Exception.Message
  exit 1
}
