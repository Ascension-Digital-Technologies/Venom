[CmdletBinding()]
param(
  [ValidateSet('dev','prod')][string]$Profile = 'prod',
  [int]$Port = 8081,
  [switch]$NoOpen
)
& (Join-Path $PSScriptRoot 'build-and-serve-example.ps1') -Example 'nova-trade' -Profile $Profile -Port $Port -NoOpen:$NoOpen
exit $LASTEXITCODE
