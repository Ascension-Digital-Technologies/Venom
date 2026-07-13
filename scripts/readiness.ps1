param(
  [Parameter(Mandatory=$true)][string]$Site,
  [string]$Dist,
  [string]$Venom = "venom",
  [ValidateSet("text","json")][string]$Format = "text",
  [switch]$RequireRealEngine,
  [switch]$Strict
)
$root = Split-Path -Parent $PSScriptRoot
$argsList = @("$root\tools\production_readiness.py", $Site, "--venom", $Venom, "--format", $Format)
if ($Dist) { $argsList += @("--dist", $Dist) }
if ($RequireRealEngine) { $argsList += "--require-real-engine" }
if ($Strict) { $argsList += "--strict" }
python @argsList
exit $LASTEXITCODE
