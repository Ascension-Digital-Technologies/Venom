param(
  [Parameter(Mandatory=$true)][string]$Release,
  [switch]$Force,
  [switch]$RequireSignature
)
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$Py=Get-Command python -ErrorAction SilentlyContinue
if(-not $Py){$Py=Get-Command py -ErrorAction SilentlyContinue}
if(-not $Py){throw 'Python 3 is required to install Venom.'}
$Args=@((Join-Path $Root 'tools\install_release.py'),'install',$Release)
if($Force){$Args+='--force'}
if($RequireSignature){$Args+='--require-signature'}
& $Py.Source @Args
exit $LASTEXITCODE
