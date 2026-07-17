param(
  [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')][string]$Config='Release',
  [string]$BuildDir='build',
  [switch]$Werror,
  [switch]$FullQuickJS
)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'internal/console.ps1')
$Root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$Dir=Join-Path $Root $BuildDir

try {
  Write-VenomBanner -Title 'Native build' -Subtitle "$Config configuration"
  Write-VenomInfo "Source: $Root"
  Write-VenomInfo "Build:  $Dir"

  $configure=@('-S',$Root,'-B',$Dir,"-DCMAKE_BUILD_TYPE=$Config")
  if($Werror){$configure+='-DVENOM_WARNINGS_AS_ERRORS=ON'}
  if($FullQuickJS){$configure+='-DVENOM_FULL_QUICKJS_TESTS=ON'}

  Invoke-VenomExternal -Program 'cmake' -Arguments $configure -Description 'Configure CMake project'
  Invoke-VenomExternal -Program 'cmake' -Arguments @('--build',$Dir,'--config',$Config,'--parallel') -Description 'Compile Venom and runtime targets'
  Write-VenomSuccess "Build completed: $Dir"
  exit 0
} catch {
  Write-VenomFailure $_.Exception.Message
  exit 1
}
