param(
  [string]$BuildRoot = "",
  [string]$TrainingSite = ""
)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
if (-not $BuildRoot) { $BuildRoot = Join-Path $Root "build\pgo" }
if (-not $TrainingSite) { $TrainingSite = Join-Path $Root "tests\fixtures\production-site" }
$Profiles = Join-Path $BuildRoot "profiles"; $Generate = Join-Path $BuildRoot "generate"; $Use = Join-Path $BuildRoot "use"; $Dist = Join-Path $BuildRoot "training-dist"
Remove-Item $Generate,$Use,$Profiles,$Dist -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $Profiles -Force | Out-Null
cmake -S $Root -B $Generate -DVENOM_PGO_MODE=generate -DVENOM_PGO_DIR=$Profiles -DBUILD_TESTING=OFF
cmake --build $Generate --config Release --parallel
$Bin = Join-Path $Generate "Release\venom.exe"; if (-not (Test-Path $Bin)) { $Bin = Join-Path $Generate "venom.exe" }
& $Bin build $TrainingSite --out $Dist --vendor-cache (Join-Path $Root "tests\fixtures\remote-cache") --offline
& $Bin verify-runtime $Dist --target browser --require-real-engine; if ($LASTEXITCODE -ne 0) { Write-Warning "Training runtime verification was unavailable; profile data from build workload is retained." }
cmake -S $Root -B $Use -DVENOM_PGO_MODE=use -DVENOM_PGO_DIR=$Profiles -DBUILD_TESTING=OFF
cmake --build $Use --config Release --parallel
Write-Host "[venom] PGO optimized build: $Use"
