$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
& python (Join-Path $Root 'tools/verify_compatibility_evidence.py') @args
exit $LASTEXITCODE
