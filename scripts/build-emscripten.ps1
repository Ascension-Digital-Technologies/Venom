param(
  [switch]$PreflightOnly,
  [switch]$AllowMissing,
  [switch]$SkipDownload,
  [switch]$SkipSetup,
  [switch]$SkipBuild,
  [switch]$SkipFinalVerify,
  [switch]$SkipNativeRebuild,
  [switch]$SkipRuntimeSmoke,
  [string]$OutDir = "",
  [string]$WasmOutDir = "",
  [string]$EmsdkDir = "",
  [string]$Version = "latest",
  [string]$Emcc = ""
)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$argsList = @("--repo-root", $Root)
if ($PreflightOnly) { $argsList += "--preflight-only" }
if ($AllowMissing) { $argsList += "--allow-missing" }
if ($SkipDownload) { $argsList += "--skip-download" }
if ($SkipSetup) { $argsList += "--skip-setup" }
if ($SkipBuild) { $argsList += "--skip-build" }
if ($SkipFinalVerify) { $argsList += "--skip-final-verify" }
if ($SkipNativeRebuild) { $argsList += "--skip-native-rebuild" }
if ($SkipRuntimeSmoke) { $argsList += "--skip-runtime-smoke" }
if ($OutDir -ne "") { $argsList += @("--out-dir", $OutDir) }
if ($WasmOutDir -ne "") { $argsList += @("--wasm-out-dir", $WasmOutDir) }
if ($EmsdkDir -ne "") { $argsList += @("--emsdk-dir", $EmsdkDir) }
if ($Version -ne "") { $argsList += @("--version", $Version) }
if ($Emcc -ne "") { $argsList += @("--emcc", $Emcc) }
python (Join-Path $Root "tools\build_emscripten.py") @argsList
