function Resolve-VenomExecutable {
  [CmdletBinding()]
  param(
    [Parameter(Mandatory=$true)][string]$Root,
    [Parameter(Mandatory=$true)][string]$BuildDir,
    [Parameter(Mandatory=$true)][string]$Config
  )

  $buildRoot = if ([IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir
  } else {
    Join-Path $Root $BuildDir
  }

  $candidates = @(
    (Join-Path $buildRoot "$Config/venom.exe"),
    (Join-Path $buildRoot 'venom.exe'),
    (Join-Path $buildRoot "$Config/venom"),
    (Join-Path $buildRoot 'venom')
  )

  foreach ($candidate in $candidates) {
    if (Test-Path -LiteralPath $candidate -PathType Leaf) {
      return (Resolve-Path -LiteralPath $candidate).Path
    }
  }

  $rendered = ($candidates | ForEach-Object { "  - $_" }) -join [Environment]::NewLine
  throw "Unable to locate the Venom executable. Checked:$([Environment]::NewLine)$rendered"
}
