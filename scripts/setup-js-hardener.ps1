[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Tool = Join-Path $Root 'tools\js-hardener'
if (-not (Get-Command node -ErrorAction SilentlyContinue)) { throw 'Node.js 20 or newer is required for release JavaScript hardening.' }
if (-not (Get-Command npm -ErrorAction SilentlyContinue)) { throw 'npm is required for release JavaScript hardening.' }
Push-Location $Tool
try { npm install --ignore-scripts --no-audit --no-fund } finally { Pop-Location }
Write-Host '[venom] release JavaScript hardener ready'
