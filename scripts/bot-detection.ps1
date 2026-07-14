[CmdletBinding()]
param(
  [ValidateSet('dev','prod')][string]$Profile = 'prod',
  [int]$Port = 8082,
  [switch]$NoOpen
)
& (Join-Path $PSScriptRoot 'build-and-serve-example.ps1') -Example 'bot-detection' -Profile $Profile -Port $Port -NoOpen:$NoOpen
exit $LASTEXITCODE
