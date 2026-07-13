param([Parameter(ValueFromRemainingArguments=$true)][string[]]$Arguments)
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
& python (Join-Path $Root "tools\benchmark_runtime.py") @Arguments
exit $LASTEXITCODE
