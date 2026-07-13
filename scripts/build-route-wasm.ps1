param([string]$Clang = "", [string]$OutDir = "")
$Root = Split-Path -Parent $PSScriptRoot
$Args = @("$Root\tools\build_route_wasm.py", "--repo-root", $Root)
if ($Clang) { $Args += @("--clang", $Clang) }
if ($OutDir) { $Args += @("--out-dir", $OutDir) }
& python @Args
if ($LASTEXITCODE -ne 0) { throw "Route VM WASM build failed with exit code $LASTEXITCODE" }
