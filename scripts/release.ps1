param(
  [string]$BuildDir = "build/release",
  [string]$Dist = "build/qualified-chess-dist",
  [UInt32]$Seed = 1350001,
  [ValidateSet("none","chromium","firefox","webkit","all")][string]$Browser = "chromium",
  [string]$PrivateKey = $env:VENOM_RELEASE_PRIVATE_KEY_FILE,
  [string]$PublicKey = $env:VENOM_RELEASE_PUBLIC_KEY_FILE,
  [string]$KeyId = $env:VENOM_RELEASE_KEY_ID,
  [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'resolve-venom.ps1')
if ([string]::IsNullOrWhiteSpace($PrivateKey) -or -not (Test-Path $PrivateKey)) {
  throw 'Stable release packaging requires -PrivateKey or VENOM_RELEASE_PRIVATE_KEY_FILE pointing to an Ed25519 private-key PEM.'
}
if ([string]::IsNullOrWhiteSpace($PublicKey) -or -not (Test-Path $PublicKey)) {
  throw 'Stable release packaging requires -PublicKey or VENOM_RELEASE_PUBLIC_KEY_FILE pointing to an Ed25519 public-key PEM.'
}
python (Join-Path $Root 'tools/final_release_gate.py') --repo-root $Root --run-smoke-tests
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not $SkipBuild) {
  & (Join-Path $PSScriptRoot 'build.ps1') -BuildDir $BuildDir -Config Release
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
$Venom = Resolve-VenomExecutable -Root $Root -BuildDir $BuildDir -Config 'Release'
if (-not (Test-Path $Venom)) { throw "Venom executable not found under $BuildDir" }
python (Join-Path $Root 'tools/release_qualification.py') --venom $Venom --dist $Dist --seed $Seed --browser $Browser
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$PackageArgs = @('--build-dir', (Join-Path $Root $BuildDir), '--out', (Join-Path $Root 'build/release-package'), '--archive', 'zip', '--release-channel', 'stable', '--sign', 'ed25519', '--private-key', $PrivateKey, '--public-key', $PublicKey)
if (-not [string]::IsNullOrWhiteSpace($KeyId)) { $PackageArgs += @('--key-id', $KeyId) }
& (Join-Path $PSScriptRoot 'package-release.ps1') @PackageArgs
exit $LASTEXITCODE
