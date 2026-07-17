param(
  [Parameter(ValueFromRemainingArguments=$true)]
  [string[]]$PackagerArgs
)
$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path (Join-Path $ScriptDir '..\..')
$PrivateKey = $env:VENOM_RELEASE_PRIVATE_KEY_FILE
$PublicKey = $env:VENOM_RELEASE_PUBLIC_KEY_FILE
if ([string]::IsNullOrWhiteSpace($PrivateKey)) { throw 'VENOM_RELEASE_PRIVATE_KEY_FILE must point to the offline Ed25519 private key PEM.' }
if ([string]::IsNullOrWhiteSpace($PublicKey)) { throw 'VENOM_RELEASE_PUBLIC_KEY_FILE must point to the trusted Ed25519 public key PEM.' }
if (-not (Test-Path -LiteralPath $PrivateKey -PathType Leaf)) { throw "Private key file not found: $PrivateKey" }
if (-not (Test-Path -LiteralPath $PublicKey -PathType Leaf)) { throw "Public key file not found: $PublicKey" }
$Python = Get-Command python -ErrorAction SilentlyContinue
if (-not $Python) { $Python = Get-Command py -ErrorAction SilentlyContinue }
if (-not $Python) { throw 'Python 3 is required to publish a stable Venom release.' }
$Prefix = @()
if ($Python.Name -in @('py.exe','py')) { $Prefix = @('-3') }
& $Python.Source @Prefix (Join-Path $RepoRoot 'tools\final_release_gate.py') '--repo-root' $RepoRoot '--run-smoke-tests'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$Args = @(
  (Join-Path $RepoRoot 'tools\package_release.py'),
  '--repo-root', $RepoRoot,
  '--release-channel', 'stable',
  '--sign', 'ed25519',
  '--private-key', $PrivateKey,
  '--public-key', $PublicKey
) + $PackagerArgs
& $Python.Source @Prefix @Args
exit $LASTEXITCODE
