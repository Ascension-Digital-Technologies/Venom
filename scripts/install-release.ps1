param([Parameter(ValueFromRemainingArguments=$true)][string[]]$Arguments)
$Root = Split-Path -Parent $PSScriptRoot
$Python = if ($env:PYTHON) { $env:PYTHON } else { "python" }
& $Python (Join-Path $Root "tools/install_release.py") @Arguments
exit $LASTEXITCODE
