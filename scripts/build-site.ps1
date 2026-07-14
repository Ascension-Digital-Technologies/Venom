param(
  [string]$Site='examples/protected-chess',
  [string]$Dist='dist',
  [string]$BuildDir='build',
  [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')][string]$Config='Release',
  [ValidateSet('dev','prod')][string]$Profile='prod'
)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'internal/console.ps1')
$Root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'resolve-venom.ps1')

try {
  Write-VenomBanner -Title 'Website build' -Subtitle "$Profile profile"
  $SitePath=if([IO.Path]::IsPathRooted($Site)){$Site}else{Join-Path $Root $Site}
  $DistPath=if([IO.Path]::IsPathRooted($Dist)){$Dist}else{Join-Path $Root $Dist}
  Write-VenomInfo "Site:   $SitePath"
  Write-VenomInfo "Output: $DistPath"

  try {
    $Exe=Resolve-VenomExecutable -Root $Root -BuildDir $BuildDir -Config $Config
  } catch {
    Write-VenomWarning 'Venom executable was not found; building it now.'
    & (Join-Path $PSScriptRoot 'build.ps1') -Config $Config -BuildDir $BuildDir
    if($LASTEXITCODE -ne 0){throw "Native build failed with exit code $LASTEXITCODE"}
    $Exe=Resolve-VenomExecutable -Root $Root -BuildDir $BuildDir -Config $Config
  }

  Invoke-VenomExternal -Program $Exe -Arguments @('build',$SitePath,'--out',$DistPath,'--profile',$Profile) -Description 'Compile protected distribution'
  if($Profile -eq 'prod'){
    Invoke-VenomExternal -Program 'python' -Arguments @((Join-Path $PSScriptRoot 'check-production-leaks.py'),$DistPath) -Description 'Scan production output for source leakage'
  }
  Write-VenomSuccess "Distribution ready: $DistPath"
  exit 0
} catch {
  Write-VenomFailure $_.Exception.Message
  exit 1
}
