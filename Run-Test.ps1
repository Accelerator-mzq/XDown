<#
.SYNOPSIS
XDown Automated Test Runner
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$Module,

    [Parameter(Mandatory=$false)]
    [string]$Param = ""
)

Write-Host "========================================="
Write-Host " Starting XDown Automated Test..."
Write-Host " Module: $Module"
Write-Host " Param:  $Param"
Write-Host "========================================="

# 1. Find the executable
$exePath = Get-ChildItem -Path .\build -Filter "appXDown.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1

if (-not $exePath) {
    Write-Host "[ERROR] Cannot find appXDown.exe! Please ensure CMake build was successful." -ForegroundColor Red
    Write-Host "`n[TEST_RESULT: FAILED]"
    exit 1
}

Write-Host "Found executable: $($exePath.FullName)"

# 2. Execute based on module
try {
    if ($Module -eq "net") {
        & $exePath.FullName --test-net $Param
    }
    elseif ($Module -eq "db") {
        & $exePath.FullName --test-db
    }
    else {
        Write-Host "[ERROR] Unknown test module: $Module" -ForegroundColor Red
        Write-Host "`n[TEST_RESULT: FAILED]"
        exit 1
    }
}
catch {
    Write-Host "[ERROR] Exception occurred during execution: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "`n[TEST_RESULT: FAILED]"
    exit 1
}

# 3. Capture exit code
$exitCode = $LASTEXITCODE

Write-Host "-----------------------------------------"
if ($exitCode -eq 0) {
    Write-Host "[SUCCESS] Test passed (Exit Code: 0)" -ForegroundColor Green
    Write-Host "`n[TEST_RESULT: SUCCESS]"
} else {
    Write-Host "[ERROR] Test failed (Exit Code: $exitCode)" -ForegroundColor Red
    Write-Host "`n[TEST_RESULT: FAILED]"
    exit $exitCode
}