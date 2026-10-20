# Runs a native command and fails only on a non-zero exit code.
# Windows PowerShell 5.1 turns redirected native stderr (e.g. CMake dev warnings) into error records,
# which ErrorActionPreference=Stop would make terminating; so stderr is passed through as text.
function Invoke-Checked([string] $What, [scriptblock] $Command) {
    Write-Host "==> $What" -ForegroundColor Cyan
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & $Command 2>&1 | ForEach-Object { if ($_ -is [System.Management.Automation.ErrorRecord]) { "$_" } else { $_ } }
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previous
    }
    if ($exitCode -ne 0) { throw "$What failed with exit code $exitCode" }
}