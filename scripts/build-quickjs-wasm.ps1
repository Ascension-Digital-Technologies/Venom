param([string]$Artifact='', [switch]$VerifyOnly)
$ErrorActionPreference='Stop'
$Root=Resolve-Path (Join-Path $PSScriptRoot '..')
$tool=Join-Path $Root 'tools/quickjs_wasm_cutover.py'
if($VerifyOnly){python $tool --repo-root $Root --verify-embedded --require-real; exit $LASTEXITCODE}
if($Artifact -eq ''){throw 'Pass -Artifact <quickjs-runtime.wasm>, or use -VerifyOnly.'}
python $tool --repo-root $Root --artifact $Artifact --require-real
