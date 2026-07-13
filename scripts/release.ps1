param(
  [string]$BuildDir = "build/release",
  [string]$Dist = "build/qualified-chess-dist",
  [UInt32]$Seed = 1350001,
  [ValidateSet("none","chromium","firefox","webkit","all")][string]$Browser = "chromium",
  [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'resolve-venom.ps1')
python (Join-Path $Root 'tools/public_release_gate.py') $Root
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not $SkipBuild) {
  & (Join-Path $PSScriptRoot 'build.ps1') -BuildDir $BuildDir -Config Release
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
$Venom = Resolve-VenomExecutable -Root $Root -BuildDir $BuildDir -Config 'Release'
if (-not (Test-Path $Venom)) { throw "Venom executable not found under $BuildDir" }
python (Join-Path $Root 'tools/release_qualification.py') --venom $Venom --dist $Dist --seed $Seed --browser $Browser
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& (Join-Path $PSScriptRoot 'package-release.ps1') --build-dir (Join-Path $Root $BuildDir) --out (Join-Path $Root 'build/release-package') --archive zip --sign none
exit $LASTEXITCODE
