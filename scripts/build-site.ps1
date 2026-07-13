param(
  [string]$Site='examples/protected-chess',
  [string]$Dist='dist',
  [string]$BuildDir='build',
  [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')][string]$Config='Release',
  [ValidateSet('debug','browser-protect','native-protect','maximum')][string]$Profile='browser-protect',
  [switch]$NoHashed
)
$ErrorActionPreference='Stop'
$Root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'resolve-venom.ps1')

try {
  $Exe=Resolve-VenomExecutable -Root $Root -BuildDir $BuildDir -Config $Config
} catch {
  & (Join-Path $PSScriptRoot 'build.ps1') -Config $Config -BuildDir $BuildDir
  if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
  $Exe=Resolve-VenomExecutable -Root $Root -BuildDir $BuildDir -Config $Config
}

$SitePath=if([IO.Path]::IsPathRooted($Site)){$Site}else{Join-Path $Root $Site}
$DistPath=if([IO.Path]::IsPathRooted($Dist)){$Dist}else{Join-Path $Root $Dist}
$args=@('build',$SitePath,'--out',$DistPath,'--profile',$Profile)
if(!$NoHashed){$args+='--hashed'}
& $Exe @args
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
if($Profile -ne 'debug'){
  python (Join-Path $PSScriptRoot 'check-production-leaks.py') $DistPath
  if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
}
