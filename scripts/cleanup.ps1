<#
.SYNOPSIS
  Remove what Coral Controller setup added.
.DESCRIPTION
  Inside the project (always safe, rebuildable):
    build\, vcpkg_installed\, snapshots\, tools\vcpkg-cache\, tools\vcpkg build artifacts.
  System-wide (only with -SystemTools; each needs a UAC approval):
    Visual Studio 2022 Build Tools (incl. MSVC ARM64, Windows SDK, CMake, Ninja)  winget id Microsoft.VisualStudio.2022.BuildTools
    Git for Windows                                                             winget id Git.Git
  Nothing else is installed system-wide. Removing the project folder removes everything else.
.EXAMPLE
  .\scripts\cleanup.ps1 -WhatIf
  .\scripts\cleanup.ps1 -SystemTools
#>
[CmdletBinding(SupportsShouldProcess)]
param([switch] $SystemTools)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$projectArtifacts = @('build', 'vcpkg_installed', 'snapshots', 'tools\vcpkg-cache',
    'tools\vcpkg\vcpkg.exe', 'tools\vcpkg\buildtrees', 'tools\vcpkg\packages', 'tools\vcpkg\downloads')
foreach ($relative in $projectArtifacts) {
    $path = Join-Path $ProjectRoot $relative
    if ((Test-Path $path) -and $PSCmdlet.ShouldProcess($path, 'Remove')) {
        Remove-Item -Recurse -Force $path
        Write-Host "Removed $path"
    }
}
if ($SystemTools) {
    foreach ($id in @('Microsoft.VisualStudio.2022.BuildTools', 'Git.Git')) {
        if ($PSCmdlet.ShouldProcess($id, 'winget uninstall')) {
            winget uninstall --id $id -e --disable-interactivity
            if ($LASTEXITCODE -ne 0) { Write-Warning "winget uninstall $id returned $LASTEXITCODE" }
        }
    }
}