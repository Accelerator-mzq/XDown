param(
    [string]$TestExe = "D:\ClaudeProject\XDown\build\tests\test_systemtest.exe",
    [string]$ServerScript = "D:\ClaudeProject\XDown\tests\mock_http_server.py",
    [int]$WaitSeconds = 3
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  XDown SystemTest Automation Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path $TestExe)) {
    Write-Host "[ERROR] Test file not found: $TestExe" -ForegroundColor Red
    exit 1
}

$pythonCmd = $null
try {
    $pythonCmd = (Get-Command python -ErrorAction SilentlyContinue).Source
    if (-not $pythonCmd) {
        $pythonCmd = (Get-Command python3 -ErrorAction SilentlyContinue).Source
    }
} catch {}

if (-not $pythonCmd) {
    Write-Host "[ERROR] Python not found, please install Python 3.x" -ForegroundColor Red
    exit 1
}

Write-Host "[INFO] Using Python: $pythonCmd" -ForegroundColor Green
Write-Host ""

Write-Host "[INFO] Starting Mock HTTP Server..." -ForegroundColor Cyan
$serverJob = Start-Job -ScriptBlock {
    param($python, $script)
    & $python $script
} -ArgumentList $pythonCmd, $ServerScript

Start-Sleep -Seconds $WaitSeconds

Write-Host "[INFO] Waiting for server..." -ForegroundColor Cyan
$healthCheck = 0
while ($healthCheck -lt 10) {
    try {
        $response = Invoke-WebRequest -Uri "http://127.0.0.1:8080/health" -TimeoutSec 2 -ErrorAction SilentlyContinue
        if ($response.StatusCode -eq 200) {
            Write-Host "[OK] Mock HTTP Server ready" -ForegroundColor Green
            break
        }
    } catch {}
    Start-Sleep -Seconds 1
    $healthCheck++
}

if ($healthCheck -ge 10) {
    Write-Host "[ERROR] Mock HTTP Server failed to start" -ForegroundColor Red
    Stop-Job -Job $serverJob
    Remove-Job -Job $serverJob
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Running SystemTest..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$testOutput = & $TestExe 2>&1 | Out-String
$exitCode = $LASTEXITCODE

Write-Host $testOutput
Write-Host ""

Write-Host "[INFO] Stopping Mock HTTP Server..." -ForegroundColor Cyan
Stop-Job -Job $serverJob -ErrorAction SilentlyContinue
Remove-Job -Job $serverJob -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Test Result" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if ($testOutput -match "Result: PASS") {
    Write-Host "[TEST_RESULT: SUCCESS]" -ForegroundColor Green
    Write-Host "All system tests passed!" -ForegroundColor Green
    exit 0
} elseif ($testOutput -match "Result: FAIL" -or $exitCode -ne 0) {
    Write-Host "[TEST_RESULT: FAILED]" -ForegroundColor Red
    Write-Host "System test failed, check output above" -ForegroundColor Red
    exit 1
} else {
    Write-Host "[TEST_RESULT: UNKNOWN]" -ForegroundColor Yellow
    Write-Host "Cannot determine test result" -ForegroundColor Yellow
    exit 1
}
