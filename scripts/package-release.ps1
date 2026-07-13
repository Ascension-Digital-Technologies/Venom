param(
  [Parameter(ValueFromRemainingArguments=$true)]
  [string[]]$ArgsForPackager
)
$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path (Join-Path $ScriptDir '..')
$Python = Get-Command python -ErrorAction SilentlyContinue
if (-not $Python) { $Python = Get-Command py -ErrorAction SilentlyContinue }
if (-not $Python) { throw 'Python 3 is required to package a release.' }
$BaseArgs = @((Join-Path $RepoRoot 'tools/package_release.py'), '--repo-root', $RepoRoot)
if ($Python.Name -eq 'py.exe' -or $Python.Name -eq 'py') {
  & $Python.Source -3 @BaseArgs @ArgsForPackager
} else {
  & $Python.Source @BaseArgs @ArgsForPackager
}
exit $LASTEXITCODE
