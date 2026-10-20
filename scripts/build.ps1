<#
.SYNOPSIS
  Configure, build and run the unit + integration tests of Coral Controller (native ARM64).
.EXAMPLE
  .\scripts\build.ps1                 # Release (RelWithDebInfo), runs tests
  .\scripts\build.ps1 -Config debug   # Debug build
  .\scripts\build.ps1 -HardwareTests  # additionally run tests that need the board
#>
[CmdletBinding()]
param(
    [ValidateSet('release', 'debug')] [string] $Config = 'release',
    [switch] $SkipTests,
    [switch] $HardwareTests
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$VcpkgRoot   = Join-Path $ProjectRoot 'tools\vcpkg'
$Preset      = "arm64-$Config"

function Invoke-Checked([string] $What, [scriptblock] $Command) {
    Write-Host "==> $What" -ForegroundColor Cyan
    & $Command
    if ($LASTEXITCODE -ne 0) { throw "$What failed with exit code $LASTEXITCODE" }
}

function Enter-Arm64DevShell {
    if (Get-Command cl.exe -ErrorAction SilentlyContinue) { return }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { throw 'Visual Studio Build Tools not found. Run: winget configure -f setup\configuration.winget' }
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.ARM64 -property installationPath
    if (-not $vsPath) { throw 'MSVC ARM64 tools missing. Apply setup\configuration.winget (needs one UAC approval).' }
    Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=arm64 -host_arch=arm64' | Out-Null
}

function Initialize-Vcpkg {
    if (-not (Test-Path (Join-Path $VcpkgRoot '.git'))) {
        Invoke-Checked 'Fetch vcpkg submodule' { git -C $ProjectRoot submodule update --init tools/vcpkg }
    }
    if (-not (Test-Path (Join-Path $VcpkgRoot 'vcpkg.exe'))) {
        Invoke-Checked 'Bootstrap vcpkg' { & (Join-Path $VcpkgRoot 'bootstrap-vcpkg.bat') -disableMetrics }
    }
    $env:VCPKG_ROOT = $VcpkgRoot
    # Keep vcpkg's binary cache inside the project (default would be %LOCALAPPDATA%).
    $cache = Join-Path $ProjectRoot 'tools\vcpkg-cache'
    New-Item -ItemType Directory -Force $cache | Out-Null
    $env:VCPKG_DEFAULT_BINARY_CACHE = $cache
    $env:VCPKG_DOWNLOADS = Join-Path $VcpkgRoot 'downloads'
}

Push-Location $ProjectRoot
try {
    Enter-Arm64DevShell
    Initialize-Vcpkg
    Invoke-Checked "Configure ($Preset)" { cmake --preset $Preset }
    Invoke-Checked "Build ($Preset)"     { cmake --build --preset $Preset }
    if (-not $SkipTests) {
        Invoke-Checked 'Unit + integration tests' { ctest --preset $Preset }
    }
    if ($HardwareTests) {
        Invoke-Checked 'Hardware tests' { ctest --preset "$Preset-hardware" }
    }
    Write-Host "Done. App: build\$Preset\app\CoralController.exe" -ForegroundColor Green
}
finally { Pop-Location }