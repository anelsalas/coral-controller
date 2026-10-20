# Runs under Windows PowerShell 5.1: powershell -File tests\scripts\InvokeChecked.Tests.ps1
# A native tool writing warnings to stderr must not abort a script under ErrorActionPreference=Stop;
# only a non-zero exit code may. Redirected (2>&1), as in VS Code tasks and log captures.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\..\scripts\lib\Invoke-Checked.ps1')
$failures = 0
function Check([string] $Name, [bool] $Passed) {
    if ($Passed) { Write-Host "PASS $Name" } else { Write-Host "FAIL $Name"; $script:failures++ }
}

$threw = $false
try { Invoke-Checked 'stderr warning, exit 0' { cmd /c "echo CMake Warning (dev) 1>&2 & exit /b 0" } 2>&1 | Out-Null } catch { $threw = $true }
Check 'StderrWarningWithExitZeroDoesNotThrow' (-not $threw)

$message = ''
try { Invoke-Checked 'exit 3' { cmd /c "echo boom 1>&2 & exit /b 3" } 2>&1 | Out-Null } catch { $message = $_.Exception.Message }
Check 'NonZeroExitThrowsWithExitCode' ($message -eq 'exit 3 failed with exit code 3')

Check 'CallerErrorActionPreferenceRestored' ($ErrorActionPreference -eq 'Stop')
exit $failures