param([Parameter(ValueFromRemainingArguments=$true)][string[]]$Args)
$Root = Split-Path -Parent $PSScriptRoot
& python (Join-Path $Root 'tools\browser_performance.py') @Args
exit $LASTEXITCODE
