# Build (unless -SkipBuild) and flash right cluster.
#   .\scripts\flash_cluster.ps1
#   .\scripts\flash_cluster.ps1 -Port COM6 -Monitor
param(
    [string]$Port = "COM6",
    [switch]$SkipBuild,
    [switch]$Monitor
)
$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

. (Join-Path $PSScriptRoot "idf_env_workspace.ps1")

$logDir = Join-Path $ProjectRoot "build\log"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$py = Join-Path $env:IDF_PYTHON_ENV_PATH "Scripts\python.exe"
$idfPy = Join-Path $env:IDF_PATH "tools\idf.py"

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "prepare_flash.ps1")
}

Write-Host "Flashing right_cluster to $Port ..."
& $py $idfPy "-p$Port" flash 2>&1 | Tee-Object -FilePath (Join-Path $logDir "flash_$Port.log")
if ($LASTEXITCODE -ne 0) { throw "idf.py flash failed: $LASTEXITCODE" }

Write-Host "PASS - right cluster flashed to $Port"
if ($Monitor) {
    & $py $idfPy "-p$Port" monitor
}
