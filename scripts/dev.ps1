[CmdletBinding()]
param(
  [string]$Site = 'examples\protected-chess',
  [string]$Dist = 'dist-dev',
  [int]$Port = 8080,
  [switch]$Open,
  [string]$Config = 'Release'
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'resolve-venom.ps1')
$exe=Resolve-VenomExecutable -Root $root -Config $Config
if(-not $exe){ throw "Venom executable not found. Run scripts\build.ps1 -Config $Config first." }
$args=@('dev',(Join-Path $root $Site),'--out',(Join-Path $root $Dist),'--port',[string]$Port)
if($Open){$args+='--open'}
& $exe @args
exit $LASTEXITCODE
