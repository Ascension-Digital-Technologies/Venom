param(
  [switch]$PreflightOnly,
  [switch]$Status,
  [switch]$Verify,
  [switch]$Clean,
  [switch]$Force,
  [string]$Emcc = "",
  [string]$OutDir = "",
  [ValidateSet("release", "debug")][string]$AbiMode = "release",
  [ValidateSet("size", "balanced", "performance")][string]$OptimizationProfile = "balanced",
  [ValidateSet("small", "medium", "large")][string]$MemoryProfile = "medium"
)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
if ($OutDir -eq "") { $OutDir = Join-Path $Root "build\quickjs-wasm" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
if ($Clean -and (Test-Path -LiteralPath $OutDir)) { Remove-Item -Recurse -Force $OutDir }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$lifecycle = Join-Path $Root "tools\quickjs_runtime_lifecycle.py"

# PowerShell omits empty-string native arguments. Passing `--emcc $Emcc` when
# $Emcc is empty therefore produces a bare `--emcc`, which argparse rejects.
# Prefer the explicit parameter, then the controller-provided EMCC environment
# value, and only append --emcc when a non-empty value is available.
$requestedEmcc = $Emcc
if ([string]::IsNullOrWhiteSpace($requestedEmcc) -and -not [string]::IsNullOrWhiteSpace($env:EMCC)) {
  $requestedEmcc = $env:EMCC
}

function New-LifecycleArguments {
  param(
    [Parameter(Mandatory=$true)][string]$Command,
    [switch]$IncludeProfiles,
    [string]$Format = ""
  )
  $items = @($lifecycle, $Command, "--repo-root", [string]$Root)
  if ($Command -in @("status", "write-state")) {
    $items += @("--out-dir", [string]$OutDir)
  }
  if (-not [string]::IsNullOrWhiteSpace($requestedEmcc)) {
    $items += @("--emcc", $requestedEmcc)
  }
  if ($IncludeProfiles) {
    $items += @(
      "--abi-mode", $AbiMode,
      "--optimization-profile", $OptimizationProfile,
      "--memory-profile", $MemoryProfile
    )
  }
  if (-not [string]::IsNullOrWhiteSpace($Format)) {
    $items += @("--format", $Format)
  }
  return $items
}

if ($Status) {
  $statusArgs = New-LifecycleArguments -Command "status" -IncludeProfiles
  & python @statusArgs
  exit $LASTEXITCODE
}
if ($Verify) {
  & python (Join-Path $Root "tools\quickjs_wasm_cutover.py") --repo-root $Root --out-dir $OutDir --verify-embedded --require-real
  exit $LASTEXITCODE
}
$resolveArgs = New-LifecycleArguments -Command "resolve-emcc" -Format "path"
$resolvedEmccOutput = & python @resolveArgs
if ($LASTEXITCODE -ne 0) { throw "Unable to locate Emscripten. Run scripts\setup-emscripten.ps1 or pass -Emcc." }
$resolvedEmccPath = ($resolvedEmccOutput | Select-Object -Last 1).Trim()
if ([string]::IsNullOrWhiteSpace($resolvedEmccPath)) { throw "Emscripten resolver returned an empty compiler path." }
$requestedEmcc = $resolvedEmccPath

$statusJsonArgs = New-LifecycleArguments -Command "status" -IncludeProfiles -Format "json"
$statusJson = & python @statusJsonArgs
if ($LASTEXITCODE -ne 0) { throw "Unable to read QuickJS WASM runtime status." }
$runtimeStatus = $statusJson | ConvertFrom-Json
Write-Host "[venom] using $($runtimeStatus.emcc_source) Emscripten: $resolvedEmccPath"
if (-not $Force) {
  if ($runtimeStatus.current) {
    Write-Host "[venom] QuickJS WASM runtime is current; rebuild skipped. Use -Force to rebuild."
    & python (Join-Path $Root "tools\quickjs_wasm_cutover.py") --repo-root $Root --out-dir $OutDir --verify-embedded --require-real
    exit $LASTEXITCODE
  }
  Write-Host "[venom] QuickJS WASM runtime is stale: $($runtimeStatus.reasons -join ', ')"
}

function Invoke-VenomExternal {
  param(
    [Parameter(Mandatory=$true)][string]$Exe,
    [Parameter(ValueFromRemainingArguments=$true)][string[]]$Args
  )
  & $Exe @Args
  $code = $LASTEXITCODE
  if ($null -ne $code -and $code -ne 0) {
    throw "command failed with exit code ${code}: $Exe $($Args -join ' ')"
  }
}

$preflightManifest = Join-Path $OutDir "quickjs-wasm-preflight.txt"
$preflightJson = Join-Path $OutDir "quickjs-wasm-preflight.json"
if ($PreflightOnly) {
  Invoke-VenomExternal python (Join-Path $Root "tools\quickjs_wasm_preflight.py") `
    "--repo-root" $Root `
    "--emcc" $resolvedEmccPath `
    "--allow-missing-emcc" `
    "--manifest" $preflightManifest `
    "--json" $preflightJson
  Write-Host "[venom] wrote QuickJS WASM preflight report: $preflightManifest"
  exit 0
}
Invoke-VenomExternal python (Join-Path $Root "tools\quickjs_wasm_preflight.py") `
  "--repo-root" $Root `
  "--emcc" $resolvedEmccPath `
  "--manifest" $preflightManifest `
  "--json" $preflightJson
$emccIsPath = Test-Path -LiteralPath $resolvedEmccPath
if ((-not $emccIsPath) -and (-not (Get-Command $resolvedEmccPath -ErrorAction SilentlyContinue))) {
  throw "emcc was not found. Install/activate Emscripten, or set EMCC to emcc.exe."
}
Write-Host "[venom] using emcc: $resolvedEmccPath"
$emccExportsJson = Join-Path $OutDir "quickjs-runtime.emcc-exports.json"
if ($AbiMode -eq "release") {
  Invoke-VenomExternal python (Join-Path $Root "tools\quickjs_release_abi.py") $emccExportsJson
} else {
  $extract = @'
import json, re, sys
src, out = sys.argv[1:]
text = open(src, encoding="utf-8").read()
names = sorted(set(re.findall(r'VENOM_QJS_PUBLIC\("([A-Za-z0-9_]+)"\)', text)))
open(out, 'w', encoding='utf-8').write(json.dumps(['_' + n for n in names], indent=2))
print(f"[venom] wrote {len(names)} debug ABI exports to {out}")
'@
  $extract | python - (Join-Path $Root "src\runtime\quickjs_runtime_scaffold.c") $emccExportsJson
  if ($LASTEXITCODE -ne 0) { throw "failed to write QuickJS ABI export list" }
}
switch ($OptimizationProfile) {
  "size" { $optFlags = @("-Oz") }
  "balanced" { $optFlags = @("-O3", "-flto") }
  "performance" { $optFlags = @("-O3", "-flto", "-fno-exceptions", "-fno-rtti") }
}
switch ($MemoryProfile) {
  "small" { $initialMemory = 16777216; $maximumMemory = 33554432 }
  "medium" { $initialMemory = 33554432; $maximumMemory = 67108864 }
  "large" { $initialMemory = 67108864; $maximumMemory = 134217728 }
}
$wasm = Join-Path $OutDir "quickjs-runtime.wasm"
& $resolvedEmccPath `
  (Join-Path $Root "src\runtime\quickjs_runtime_scaffold.c") `
  (Join-Path $Root "third_party\quickjs\quickjs.c") `
  (Join-Path $Root "third_party\quickjs\quickjs-libc.c") `
  (Join-Path $Root "third_party\quickjs\libregexp.c") `
  (Join-Path $Root "third_party\quickjs\libunicode.c") `
  (Join-Path $Root "third_party\quickjs\dtoa.c") `
  "-I$(Join-Path $Root 'third_party\quickjs')" `
  "-DVENOM_ENABLE_UPSTREAM_QJS_WASM=1" `
  $(if ($AbiMode -eq "release") { "-DVENOM_QJS_MINIMAL_EXPORTS=1" } else { "-DVENOM_QJS_DEBUG_EXPORTS=1" }) `
  "-DVENOM_WASM_NATIVE_STACK_CAPACITY=4194304" `
  "-DQUICKJS_NG_BUILD" `
  "-D_GNU_SOURCE" `
  "-DNDEBUG" `
  "-g0" `
  "-ffile-prefix-map=$Root=." `
  "-fdebug-prefix-map=$Root=." `
  "-fmacro-prefix-map=$Root=." `
  @optFlags `
  "-sSTANDALONE_WASM=1" `
  "-sASSERTIONS=0" `
  "-sSAFE_HEAP=0" `
  "-sSTACK_OVERFLOW_CHECK=0" `
  "-sALLOW_MEMORY_GROWTH=1" `
  "-sSTACK_SIZE=4194304" `
  "-sINITIAL_MEMORY=$initialMemory" `
  "-sMAXIMUM_MEMORY=$maximumMemory" `
  "-sEXPORTED_FUNCTIONS=@$emccExportsJson" `
  "-Wl,--no-entry" `
  "-Wl,--strip-all" `
  "-o" $wasm
if ($LASTEXITCODE -ne 0) { throw "emcc failed with exit code $LASTEXITCODE" }
$wasmOptPath = & python $lifecycle resolve-wasm-opt --repo-root $Root --format path 2>$null
if ($LASTEXITCODE -eq 0 -and $wasmOptPath) {
  $wasmOptPath = ($wasmOptPath | Select-Object -Last 1).Trim()
  $wasmOptLevel = switch ($OptimizationProfile) { "size" { "-Oz" } "balanced" { "-O3" } "performance" { "-O4" } }
  $optimized = Join-Path $OutDir "quickjs-runtime.opt.wasm"
  # Emscripten may emit post-MVP instructions such as memory.copy, memory.fill,
  # and trunc_sat conversions. The standalone wasm-opt invocation must detect
  # and explicitly permit those features before validation and optimization.
  $wasmFeatureFlags = @(
    "--detect-features",
    "--enable-bulk-memory-opt",
    "--enable-nontrapping-float-to-int"
  )
  Write-Host "[venom] applying Binaryen $wasmOptLevel with detected Emscripten features using $wasmOptPath"
  Invoke-VenomExternal $wasmOptPath $wasmOptLevel @wasmFeatureFlags "--strip-debug" "--strip-producers" $wasm "-o" $optimized
  Move-Item -Force $optimized $wasm
} else {
  Write-Host "[venom] wasm-opt not found; keeping optimized Emscripten output."
}
Invoke-VenomExternal python (Join-Path $Root "tools\check_wasm_release_strings.py") $wasm
@("abi=$AbiMode", "optimization=$OptimizationProfile", "memory=$MemoryProfile", "initial_memory=$initialMemory", "maximum_memory=$maximumMemory") | Set-Content -Encoding ascii (Join-Path $OutDir "quickjs-build-profile.txt")
Invoke-VenomExternal python (Join-Path $Root "tools\quickjs_wasm_cutover.py") `
  "--repo-root" $Root `
  "--out-dir" $OutDir `
  "--emcc" $resolvedEmccPath `
  "--artifact" $wasm
$writeStateArgs = New-LifecycleArguments -Command "write-state" -IncludeProfiles
Invoke-VenomExternal python @writeStateArgs
Invoke-VenomExternal python (Join-Path $Root "tools\runtime_performance.py") "--repo-root" $Root "--artifact" $wasm "--json-out" (Join-Path $OutDir "runtime-performance.json")
Write-Host "[venom] built, embedded, benchmarked, and verified upstream QuickJS WASM artifact: $wasm"
