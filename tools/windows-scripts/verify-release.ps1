param(
  [Parameter(ValueFromRemainingArguments=$true)]
  [string[]]$ArgsForVerifier
)
$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path (Join-Path $ScriptDir '../..')
$Python = Get-Command python -ErrorAction SilentlyContinue
if (-not $Python) { $Python = Get-Command py -ErrorAction SilentlyContinue }
if (-not $Python) { throw 'Python 3 is required to verify a release.' }

if ($ArgsForVerifier.Count -eq 0) {
  $BaseArgs = @((Join-Path $RepoRoot 'tools/core_release_closure.py'), '--repo-root', $RepoRoot, '--build')
} else {
  $BaseArgs = @((Join-Path $RepoRoot 'tools/verify_release.py'))
}

if ($Python.Name -eq 'py.exe' -or $Python.Name -eq 'py') {
  & $Python.Source -3 @BaseArgs @ArgsForVerifier
} else {
  & $Python.Source @BaseArgs @ArgsForVerifier
}
exit $LASTEXITCODE
