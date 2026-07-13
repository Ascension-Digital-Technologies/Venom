param([switch]$RequireReal, [string]$JsonOut = "")
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$argsList = @((Join-Path $Root "tools\release_closure.py"), "--repo-root", $Root)
if ($RequireReal) { $argsList += "--require-real" }
if ($JsonOut -ne "") { $argsList += @("--json-out", $JsonOut) }
python @argsList
exit $LASTEXITCODE
