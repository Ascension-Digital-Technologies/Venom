param(
    [string]$Config = "Release",
    [ValidateSet("all", "chromium", "firefox", "webkit")]
    [string]$Browser = "all",
    [ValidateSet("dev", "prod")]
    [string]$Profile = "dev",
    [string]$Out = "build/compatibility",
    [switch]$SkipBuild
)
$ErrorActionPreference = "Stop"
$Repo = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot "resolve-venom.ps1")
$Exe = Resolve-VenomExecutable -RepoRoot $Repo -Config $Config
if (-not $Exe) { throw "Venom executable not found. Run scripts/build.ps1 -Config $Config first." }
$Args = @(
    (Join-Path $Repo "tools/run_compatibility_suite.py"),
    "--venom", $Exe,
    "--browser", $Browser,
    "--profile", $Profile,
    "--out", $Out
)
if ($SkipBuild) { $Args += "--skip-build" }
& python @Args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
