<#
.SYNOPSIS
  Build the board firmware (camera_stream_binary + SDK elf_loader) with the coralmicro SDK.
.DESCRIPTION
  Uses CMake + Ninja from Visual Studio 2022 Build Tools and the Arm GNU toolchain that the SDK
  toolchain file downloads (gcc-arm-none-eabi-9-2020-q2, Arm's official win32 zip) into
  third_party\coralmicro\third_party\toolchain-win. Output: build\firmware.
#>
[CmdletBinding()]
param([switch] $Clean)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot 'build\firmware'
$Targets = @('camera_stream_binary', 'elf_loader')

function Invoke-Checked([string] $What, [scriptblock] $Command) {
    Write-Host "==> $What" -ForegroundColor Cyan
    & $Command
    if ($LASTEXITCODE -ne 0) { throw "$What failed with exit code $LASTEXITCODE" }
}

function Add-VsCMakeAndNinjaToPath {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vsPath = & $vswhere -latest -products * -property installationPath
    if (-not $vsPath) { throw 'Visual Studio Build Tools not found. Apply setup\configuration.winget.' }
    $cmakeRoot = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake'
    $env:Path = "$cmakeRoot\CMake\bin;$cmakeRoot\Ninja;$env:Path"
}

Push-Location $ProjectRoot
try {
    if (-not (Test-Path 'third_party\coralmicro\third_party\nxp\rt1176-sdk\devices')) {
        Invoke-Checked 'Fetch coralmicro SDK submodules' {
            git -c core.longpaths=true submodule update --init --recursive third_party/coralmicro
        }
    }
    Add-VsCMakeAndNinjaToPath
    if ($Clean -and (Test-Path $BuildDir)) { Remove-Item -Recurse -Force $BuildDir }
    Invoke-Checked 'Configure firmware' {
        cmake -S firmware -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Release
    }
    Invoke-Checked 'Build firmware' { cmake --build $BuildDir --target $Targets }
    $elf = Join-Path $BuildDir 'camera_stream_binary\camera_stream_binary.stripped'
    if (-not (Test-Path $elf)) { throw "Expected firmware image missing: $elf" }
    Write-Host "Done. Firmware: $elf" -ForegroundColor Green
}
finally { Pop-Location }