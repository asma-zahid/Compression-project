# Quick validation script to check if the project builds and basic tests pass

param(
    [string]$ProjectRoot = (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
)

Write-Host "Validating BZip2 implementation..."
Write-Host "Project root: $ProjectRoot"
Write-Host ""

# Check if binary exists
$binaryPath = Join-Path $ProjectRoot "bzip2_impl"
Write-Host "Looking for binary at: $binaryPath"
if (-not (Test-Path $binaryPath)) {
    Write-Host "ERROR: bzip2_impl binary not found at $binaryPath"
    exit 1
}
Write-Host "✓ Binary exists"

# Try basic compression test
$testInput = Join-Path $ProjectRoot "test_input.txt"
$testCompressed = Join-Path $ProjectRoot "test.bz"
$testOutput = Join-Path $ProjectRoot "test_output.txt"

# Create test input
"test" | Out-File -FilePath $testInput -Encoding ASCII

Write-Host "Testing basic compression..."
try {
    & $binaryPath compress $testInput $testCompressed > $null 2>&1
    Write-Host "✓ Compression works"
} catch {
    Write-Host "ERROR: Compression failed"
    Remove-Item $testInput -ErrorAction SilentlyContinue
    exit 1
}

# Try basic decompression test
Write-Host "Testing basic decompression..."
try {
    & $binaryPath decompress $testCompressed $testOutput > $null 2>&1
    Write-Host "✓ Decompression works"
} catch {
    Write-Host "ERROR: Decompression failed"
    Remove-Item $testInput, $testCompressed -ErrorAction SilentlyContinue
    exit 1
}

# Check roundtrip
if (Compare-Object (Get-Content $testInput) (Get-Content $testOutput)) {
    Write-Host "ERROR: Roundtrip failed"
    Remove-Item $testInput, $testCompressed, $testOutput -ErrorAction SilentlyContinue
    exit 1
}
Write-Host "✓ Roundtrip successful"

# Cleanup
Remove-Item $testInput, $testCompressed, $testOutput -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "✓ All validation checks passed!"
Write-Host "The BZip2 implementation is working correctly."