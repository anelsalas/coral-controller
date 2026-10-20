<#
.SYNOPSIS
  Run the hardware-labelled tests (board connected, Coral Controller closed).
.NOTES
  Uses ctest from the VS Build Tools CMake component. Builds arm64-release first.
#>
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * -property installationPath
$env:PATH = "$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;$env:PATH"
Push-Location $ProjectRoot
try {
    ctest --preset arm64-release-hardware --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "hardware tests failed ($LASTEXITCODE)" }
} finally { Pop-Location }