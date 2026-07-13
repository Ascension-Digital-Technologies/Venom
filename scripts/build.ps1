param(
  [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')][string]$Config='Release',
  [string]$BuildDir='build',
  [switch]$Werror,
  [switch]$FullQuickJS
)
$ErrorActionPreference='Stop'
$Root=Resolve-Path (Join-Path $PSScriptRoot '..')
$Dir=Join-Path $Root $BuildDir
$args=@('-S',$Root,'-B',$Dir,"-DCMAKE_BUILD_TYPE=$Config")
if($Werror){$args+='-DVENOM_WARNINGS_AS_ERRORS=ON'}
if($FullQuickJS){$args+='-DVENOM_FULL_QUICKJS_TESTS=ON'}
& cmake @args
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& cmake --build $Dir --config $Config --parallel
exit $LASTEXITCODE
