[CmdletBinding()]
param(
  [ValidateSet('dev','prod')][string]$Profile = 'prod',
  [int]$Port = 8080,
  [switch]$NoOpen
)
& (Join-Path $PSScriptRoot 'build-and-serve-example.ps1') -Example 'protected-chess' -Profile $Profile -Port $Port -NoOpen:$NoOpen
exit $LASTEXITCODE
