[CmdletBinding()]
param(
  [switch]$Force,
  [switch]$NoPause
)

$ErrorActionPreference = 'Stop'
$ExitCode = 0

function Wait-OnFailure {
  param([string]$Message)
  Write-Host ''
  Write-Host '[venom] JavaScript hardener setup failed.' -ForegroundColor Red
  Write-Host $Message -ForegroundColor Red
  Write-Host ''
  Write-Host 'Run this command from an open PowerShell window to keep the full output visible:' -ForegroundColor Yellow
  Write-Host '  powershell -NoExit -ExecutionPolicy Bypass -File .\scripts\setup-js-hardener.ps1 -Force' -ForegroundColor Yellow
  if (-not $NoPause -and $Host.Name -notmatch 'ServerRemoteHost') {
    Write-Host ''
    Write-Host 'Press any key to close . . .' -ForegroundColor Cyan
    try { $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown') } catch { Read-Host | Out-Null }
  }
}

try {
  $Root = Split-Path -Parent $PSScriptRoot
  $Tool = Join-Path $Root 'tools\js-hardener'
  $PackageJson = Join-Path $Tool 'package.json'
  $NodeModules = Join-Path $Tool 'node_modules'
  $LockFile = Join-Path $Tool 'package-lock.json'
  
  function Invoke-Checked {
    param(
      [Parameter(Mandatory = $true)][string]$Program,
      [Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments
    )
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
      throw "$Program failed with exit code $LASTEXITCODE"
    }
  }
  
  $Node = Get-Command node -ErrorAction SilentlyContinue
  $Npm = Get-Command npm.cmd -ErrorAction SilentlyContinue
  if (-not $Npm) { $Npm = Get-Command npm -ErrorAction SilentlyContinue }
  if (-not $Node) { throw 'Node.js 20 or newer is required for release JavaScript hardening.' }
  if (-not $Npm) { throw 'npm is required for release JavaScript hardening.' }
  if (-not (Test-Path -LiteralPath $PackageJson)) { throw "Missing JS hardener package manifest: $PackageJson" }
  
  $NodeMajorText = & $Node.Source -p "Number(process.versions.node.split('.')[0])"
  if ($LASTEXITCODE -ne 0) { throw 'Unable to determine the installed Node.js version.' }
  $NodeMajor = 0
  if (-not [int]::TryParse(($NodeMajorText | Select-Object -First 1).ToString().Trim(), [ref]$NodeMajor) -or $NodeMajor -lt 20) {
    throw "Node.js 20 or newer is required. Detected: $(& $Node.Source --version)"
  }
  
  Push-Location $Tool
  try {
    $NeedsInstall = $Force -or -not (Test-Path -LiteralPath (Join-Path $NodeModules 'terser\package.json')) -or
      -not (Test-Path -LiteralPath (Join-Path $NodeModules 'javascript-obfuscator\package.json'))
  
    if (-not $NeedsInstall) {
      & $Node.Source -e "await Promise.all([import('terser'), import('javascript-obfuscator')]);"
      if ($LASTEXITCODE -ne 0) { $NeedsInstall = $true }
    }
  
    if ($NeedsInstall) {
      Write-Host '[venom] installing pinned JavaScript hardener dependencies...'
      if (Test-Path -LiteralPath $NodeModules) {
        Remove-Item -LiteralPath $NodeModules -Recurse -Force
      }
  
      # Older source archives could contain a lockfile generated against a private
      # registry. Remove only those non-portable locks; npm will create a clean one.
      if (Test-Path -LiteralPath $LockFile) {
        $LockText = Get-Content -LiteralPath $LockFile -Raw
        if ($LockText -match 'applied-caas-gateway|internal\.api\.openai\.org') {
          Remove-Item -LiteralPath $LockFile -Force
        }
      }
  
      Invoke-Checked -Program $Npm.Source -Arguments @('install', '--ignore-scripts', '--no-audit', '--no-fund', '--save-exact')
    }
  
    Invoke-Checked -Program $Node.Source -Arguments @('-e', "const [t,o]=await Promise.all([import('terser'),import('javascript-obfuscator')]); if(typeof t.minify!=='function'||!(o.default||o).obfuscate) process.exit(2);")
  
    $TempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('venom-js-hardener-check-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $TempRoot -Force | Out-Null
    try {
      $InputFile = Join-Path $TempRoot 'input.js'
      $OutputFile = Join-Path $TempRoot 'output.js'
      Set-Content -LiteralPath $InputFile -Value 'function add(a,b){return a+b;}globalThis.__venomHardenerCheck=add(2,3);' -Encoding UTF8
      Invoke-Checked -Program $Node.Source -Arguments @((Join-Path $Tool 'harden.mjs'), $InputFile, $OutputFile, 'runtime', '1364')
      if (-not (Test-Path -LiteralPath $OutputFile) -or (Get-Item -LiteralPath $OutputFile).Length -eq 0) {
        throw 'JavaScript hardener self-test produced no output.'
      }
      Invoke-Checked -Program $Node.Source -Arguments @('--check', $OutputFile)
    } finally {
      Remove-Item -LiteralPath $TempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
  } finally {
    Pop-Location
  }
  
  Write-Host '[venom] release JavaScript hardener installed and verified'
}
catch {
  $ExitCode = 1
  $Details = $_.Exception.Message
  if ($_.ScriptStackTrace) {
    $Details += "`n`nPowerShell stack:`n" + $_.ScriptStackTrace
  }
  Wait-OnFailure -Message $Details
}

exit $ExitCode
