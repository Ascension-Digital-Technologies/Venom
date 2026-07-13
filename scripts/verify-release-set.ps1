$Root = Split-Path -Parent $PSScriptRoot
& python (Join-Path $Root 'tools/verify_release_set.py') @args
exit $LASTEXITCODE
