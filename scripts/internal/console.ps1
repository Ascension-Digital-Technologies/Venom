Set-StrictMode -Version Latest

$script:VenomConsoleUseColor = $Host.Name -notmatch 'ServerRemoteHost' -and -not $env:NO_COLOR

function Write-VenomLine {
  param(
    [Parameter(Mandatory=$true)][string]$Label,
    [Parameter(Mandatory=$true)][string]$Message,
    [ConsoleColor]$Color = [ConsoleColor]::Gray
  )
  $prefix = ('[{0}]' -f $Label.ToUpperInvariant()).PadRight(10)
  if ($script:VenomConsoleUseColor) {
    Write-Host $prefix -NoNewline -ForegroundColor $Color
    Write-Host " $Message"
  } else {
    Write-Host "$prefix $Message"
  }
}

function Write-VenomBanner {
  param(
    [Parameter(Mandatory=$true)][string]$Title,
    [string]$Subtitle = ''
  )
  Write-Host ''
  if ($script:VenomConsoleUseColor) {
    Write-Host 'VENOM' -ForegroundColor Green -NoNewline
    Write-Host "  $Title" -ForegroundColor White
  } else {
    Write-Host "VENOM  $Title"
  }
  if ($Subtitle) { Write-Host "       $Subtitle" -ForegroundColor DarkGray }
  Write-Host ('-' * 72) -ForegroundColor DarkGray
}

function Write-VenomStep { param([string]$Message) Write-VenomLine -Label 'STEP' -Message $Message -Color Cyan }
function Write-VenomInfo { param([string]$Message) Write-VenomLine -Label 'INFO' -Message $Message -Color Gray }
function Write-VenomSuccess { param([string]$Message) Write-VenomLine -Label 'SUCCESS' -Message $Message -Color Green }
function Write-VenomWarning { param([string]$Message) Write-VenomLine -Label 'WARNING' -Message $Message -Color Yellow }
function Write-VenomFailure { param([string]$Message) Write-VenomLine -Label 'ERROR' -Message $Message -Color Red }

function Invoke-VenomExternal {
  [CmdletBinding()]
  param(
    [Parameter(Mandatory=$true)][string]$Program,
    [Parameter()][string[]]$Arguments = @(),
    [string]$Description = ''
  )
  if ($Description) { Write-VenomStep $Description }
  & $Program @Arguments
  $code = $LASTEXITCODE
  if ($null -eq $code) { $code = 0 }
  if ($code -ne 0) { throw "$Program failed with exit code $code" }
}
