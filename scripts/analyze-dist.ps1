param(
  [Parameter(ValueFromRemainingArguments=$true)]
  [string[]]$Args
)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
python (Join-Path $Root "tools\analyze_dist.py") --repo-root $Root @Args
