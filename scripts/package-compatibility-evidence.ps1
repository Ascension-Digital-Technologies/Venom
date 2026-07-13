$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
& python (Join-Path $Root 'tools/package_compatibility_evidence.py') @args
exit $LASTEXITCODE
