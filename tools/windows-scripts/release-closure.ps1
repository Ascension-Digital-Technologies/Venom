param(
  [Parameter(ValueFromRemainingArguments=$true)]
  [string[]]$ClosureArgs
)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$Python = Get-Command python -ErrorAction SilentlyContinue
if (-not $Python) { $Python = Get-Command py -ErrorAction SilentlyContinue }
if (-not $Python) { throw 'Python 3 is required to run the release closure.' }
$Prefix = @()
if ($Python.Name -in @('py.exe','py')) { $Prefix = @('-3') }
& $Python.Source @Prefix (Join-Path $Root 'tools/run_release_closure.py') '--repo-root' $Root @ClosureArgs
exit $LASTEXITCODE
