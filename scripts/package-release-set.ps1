$Root = Split-Path -Parent $PSScriptRoot
& python (Join-Path $Root 'tools/package_release_set.py') @args
exit $LASTEXITCODE
