param(
  [switch]$CheckOnly,
  [switch]$AllowMissing,
  [switch]$SkipDownload,
  [string]$EmsdkDir = "",
  [string]$Version = "latest",
  [string]$Emcc = "",
  [string]$OutDir = ""
)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
if ($OutDir -eq "") { $OutDir = Join-Path $Root "build\emscripten-setup" }
$argsList = @("--repo-root", $Root, "--out-dir", $OutDir, "--version", $Version)
if ($CheckOnly) { $argsList += "--check-only" }
if ($AllowMissing) { $argsList += "--allow-missing" }
if ($SkipDownload) { $argsList += "--skip-download" }
if ($EmsdkDir -ne "") { $argsList += @("--emsdk-dir", $EmsdkDir) }
if ($Emcc -ne "") { $argsList += @("--emcc", $Emcc) }
python (Join-Path $Root "tools\setup_emscripten.py") @argsList
