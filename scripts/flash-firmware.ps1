<#
.SYNOPSIS
  Flash board firmware with Coral's flashtool.py (interim; Stage 3 replaces this with the C++ controller).
.EXAMPLE
  .\scripts\flash-firmware.ps1                    # flash build\firmware camera_stream_binary
  .\scripts\flash-firmware.ps1 -Ram               # run from RAM only (reverts on reset)
  .\scripts\flash-firmware.ps1 -RestoreTextApp    # restore the previous camera_streaming_serial
.NOTES
  Close Coral Controller first: flashing needs the board's USB interfaces.
  flashtool.py and its Python venv come from the existing checkout in $CoralMicroTools.
#>
[CmdletBinding()]
param(
    [switch] $Ram,
    [switch] $RestoreTextApp,
    [string] $CoralMicroTools = 'C:\Users\anels\Source\coralmicro'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$python = Join-Path $CoralMicroTools 'venv\Scripts\python.exe'
$flashtool = Join-Path $CoralMicroTools 'scripts\flashtool.py'

if ($RestoreTextApp) {
    $buildDir = Join-Path $CoralMicroTools 'out'
    $elf = Join-Path $buildDir 'examples\camera_streaming_serial\camera_streaming_serial.stripped'
} else {
    $buildDir = Join-Path $ProjectRoot 'build\firmware'
    $elf = Join-Path $buildDir 'camera_stream_binary\camera_stream_binary.stripped'
}
foreach ($required in @($python, $flashtool, $elf)) {
    if (-not (Test-Path $required)) { throw "Missing: $required" }
}
if (Get-CimInstance Win32_Process -Filter "Name='CoralController.exe'") {
    throw 'Coral Controller is running and may hold the board. Close it and retry.'
}

$arguments = @($flashtool, '--build_dir', $buildDir, '--elf_path', $elf)
if ($Ram) { $arguments += '--ram' }
Write-Host "==> flashtool.py $($arguments[1..($arguments.Count-1)] -join ' ')" -ForegroundColor Cyan
& $python @arguments
if ($LASTEXITCODE -ne 0) { throw "flashtool.py failed with exit code $LASTEXITCODE" }
Write-Host 'Flashed.' -ForegroundColor Green