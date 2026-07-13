param(
  [Parameter(Mandatory=$true)][string]$Dist,
  [ValidateSet('chromium','firefox','webkit','all')][string]$Browser='chromium',
  [Parameter(Mandatory=$true)][string]$Manifest,
  [string]$JsonOut=''
)
$Root = Split-Path -Parent $PSScriptRoot
$Args = @((Join-Path $Root 'tools/browser_validation.py'), $Dist, '--browser', $Browser, '--manifest', $Manifest)
if ($JsonOut) { $Args += @('--json-out', $JsonOut) }
python @Args
exit $LASTEXITCODE
