# PowerShell master test runner - runs all test suites

param(
    [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot)
)

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "BZip2 Comprehensive Test Suite (PowerShell)" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Check if binary exists
$binaryPath = Join-Path $ProjectRoot "bzip2_impl"
if (-not (Test-Path $binaryPath)) {
    Write-Host "ERROR: bzip2_impl binary not found. Please build the project first." -ForegroundColor Red
    Write-Host "Run: .\build.ps1 (if you have MinGW) or use WSL" -ForegroundColor Yellow
    exit 1
}

# Check if binary is executable (not Windows .exe)
try {
    $testOutput = & $binaryPath 2>&1
    if ($LASTEXITCODE -ne 0 -and $testOutput -match "cannot execute binary file") {
        Write-Host "ERROR: bzip2_impl is a Linux binary. Use WSL or build with MinGW." -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "ERROR: Cannot execute bzip2_impl. Please check the binary." -ForegroundColor Red
    exit 1
}

Write-Host "✓ Binary is executable" -ForegroundColor Green
Write-Host ""

# Run validation
Write-Host "1. Running Build Validation..."
$validateScript = Join-Path $PSScriptRoot "validate_build.ps1"
if (Test-Path $validateScript) {
    try {
        & $validateScript
        Write-Host "✓ Validation passed" -ForegroundColor Green
    } catch {
        Write-Host "✗ Validation failed" -ForegroundColor Red
    }
} else {
    Write-Host "Validation script not found, skipping..." -ForegroundColor Yellow
}
Write-Host ""

# Note: Unit tests require compilation, skip for now
Write-Host "2. Unit Tests - Skipped (requires compilation)" -ForegroundColor Yellow
Write-Host "   Use WSL or MinGW to run: bash tests/run_unit_tests.sh" -ForegroundColor Yellow
Write-Host ""

# Run integration tests (simplified PowerShell version)
Write-Host "3. Running Integration Tests..."

$testDataDir = Join-Path $PSScriptRoot "test_data"
$tempDir = Join-Path $PSScriptRoot "temp"
$logFile = Join-Path $PSScriptRoot "test_results.log"

if (-not (Test-Path $tempDir)) {
    New-Item -ItemType Directory -Path $tempDir | Out-Null
}

# Clear log
"" | Out-File -FilePath $logFile

function Test-Roundtrip {
    param([string]$TestName, [string]$InputFile)
    
    if (-not (Test-Path $InputFile)) {
        Write-Host "  SKIP: $TestName (input file missing)" -ForegroundColor Yellow
        return
    }
    
    $compressedFile = Join-Path $tempDir "$((Get-Item $InputFile).BaseName).bz"
    $outputFile = Join-Path $tempDir "$((Get-Item $InputFile).BaseName)_out$((Get-Item $InputFile).Extension)"
    
    try {
        # Compress
        & $binaryPath compress $InputFile $compressedFile > $null 2>&1
        if ($LASTEXITCODE -ne 0) { throw "Compression failed" }
        
        # Decompress
        & $binaryPath decompress $compressedFile $outputFile > $null 2>&1
        if ($LASTEXITCODE -ne 0) { throw "Decompression failed" }
        
        # Compare
        $inputContent = Get-Content $InputFile -Raw
        $outputContent = Get-Content $outputFile -Raw
        
        if ($inputContent -eq $outputContent) {
            $inputSize = (Get-Item $InputFile).Length
            $compSize = (Get-Item $compressedFile).Length
            $ratio = [math]::Round($inputSize / $compSize, 2)
            Write-Host "  PASS: $TestName (ratio: ${ratio}x)" -ForegroundColor Green
        } else {
            Write-Host "  FAIL: $TestName (content mismatch)" -ForegroundColor Red
        }
    } catch {
        Write-Host "  FAIL: $TestName ($($_.Exception.Message))" -ForegroundColor Red
    } finally {
        Remove-Item $compressedFile, $outputFile -ErrorAction SilentlyContinue
    }
}

# Test basic cases
Test-Roundtrip "Single Character" (Join-Path $testDataDir "single_char.txt")
Test-Roundtrip "Empty File" (Join-Path $testDataDir "empty_file.txt")
Test-Roundtrip "Pangram" (Join-Path $testDataDir "pangram.txt")
Test-Roundtrip "Repeated Characters" (Join-Path $testDataDir "repeated_chars.txt")

# Test binary file
$binaryTest = Join-Path $testDataDir "all_zeros_1000.bin"
if (Test-Path $binaryTest) {
    Test-Roundtrip "Binary Data (zeros)" $binaryTest
}

Write-Host ""
Write-Host "4. Performance Tests - Skipped (use WSL for full benchmarks)" -ForegroundColor Yellow
Write-Host ""

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Basic tests completed!" -ForegroundColor Green
Write-Host "For full test suite, use WSL or Linux environment." -ForegroundColor Yellow
Write-Host "==========================================" -ForegroundColor Cyan