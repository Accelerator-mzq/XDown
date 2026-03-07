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

# 0. Set Qt PATH dynamically to prevent STATUS_DLL_NOT_FOUND without static copies
$qt_bin = "E:\Qt\6.5.3\mingw_64\bin"
$env:PATH = "$env:PATH;$qt_bin"

# Force UTF-8 Encoding to prevent Qt console garbled characters in output
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::InputEncoding = [System.Text.Encoding]::UTF8

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

# 1.5 Start Mock Server
Write-Host "Starting Mock HTTP Server..."
$pythonExe = "python"
$mockServerPath = "$PSScriptRoot\tests\src\mock_http_server.py"
$serverJob = Start-Job -ScriptBlock {
    param($exe, $script)
    # Change to the project root directory
    Set-Location -Path (Split-Path -Path $script -Parent | Split-Path -Parent | Split-Path -Parent)
    & $exe $script
} -ArgumentList $pythonExe, $mockServerPath

# Wait 2 seconds for server to start
Start-Sleep -Seconds 2

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
Write-Host "Stopping Mock HTTP Server..."
Stop-Job $serverJob -ErrorAction SilentlyContinue
Remove-Job $serverJob -ErrorAction SilentlyContinue

if ($exitCode -eq 0) {
    Write-Host "[SUCCESS] Test passed (Exit Code: 0)" -ForegroundColor Green
    Write-Host "`n[TEST_RESULT: SUCCESS]"
} else {
    Write-Host "[ERROR] Test failed (Exit Code: $exitCode)" -ForegroundColor Red
    Write-Host "`n[TEST_RESULT: FAILED]"
    exit $exitCode
}