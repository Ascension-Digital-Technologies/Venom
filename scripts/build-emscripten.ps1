[CmdletBinding()]
param([string]$OutDir='', [string]$EmsdkDir='', [string]$Emcc='', [switch]$PreflightOnly,
      [switch]$AllowMissing, [switch]$SkipDownload, [switch]$SkipSetup, [switch]$Clean,
      [switch]$Force, [switch]$SkipNativeRebuild, [switch]$SkipRuntimeSmoke)
$Root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$a=@((Join-Path $Root 'tools/build_emscripten.py'),'--repo-root',$Root)
if($OutDir){$a+=@('--out-dir',$OutDir)}; if($EmsdkDir){$a+=@('--emsdk-dir',$EmsdkDir)}; if($Emcc){$a+=@('--emcc',$Emcc)}
foreach($x in @(@($PreflightOnly,'--preflight-only'),@($AllowMissing,'--allow-missing'),@($SkipDownload,'--skip-download'),@($SkipSetup,'--skip-setup'),@($Clean,'--clean'),@($Force,'--force'),@($SkipNativeRebuild,'--skip-native-rebuild'),@($SkipRuntimeSmoke,'--skip-runtime-smoke'))){if($x[0]){$a+=$x[1]}}
& python @a
exit $LASTEXITCODE
