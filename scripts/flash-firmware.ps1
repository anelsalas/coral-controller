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

# pyusb needs a libusb backend; the venv's libusb-package wheel bundles libusb-1.0.dll.
$libusbDir = Join-Path $CoralMicroTools 'venv\Lib\site-packages\libusb_package'
if (-not (Test-Path (Join-Path $libusbDir 'libusb-1.0.dll'))) { throw "Missing libusb-1.0.dll in $libusbDir" }
$env:PATH = "$libusbDir;$env:PATH"

$arguments = @($flashtool, '--build_dir', $buildDir, '--elf_path', $elf)
if ($Ram) { $arguments += '--ram' }
Write-Host "==> flashtool.py $($arguments[1..($arguments.Count-1)] -join ' ')" -ForegroundColor Cyan
# flashtool.py exits 0 even when it reports a missing libusb, so check its output too.
$output = & $python @arguments 2>&1 | Tee-Object -Variable output | ForEach-Object { Write-Host $_; $_ }
if ($LASTEXITCODE -ne 0) { throw "flashtool.py failed with exit code $LASTEXITCODE" }
if (($output | Out-String) -match 'cannot find libusb') { throw 'flashtool.py could not load libusb; nothing was flashed.' }
Write-Host 'Flashed.' -ForegroundColor Green