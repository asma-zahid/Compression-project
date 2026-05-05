#!/bin/bash
# Quick validation script to check if the project builds and basic tests pass

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "Validating BZip2 implementation..."
echo ""

# Check if Makefile exists
if [ ! -f "$PROJECT_ROOT/Makefile" ]; then
    echo "ERROR: Makefile not found"
    exit 1
fi

# Try to build
echo "Building project..."
if ! make -C "$PROJECT_ROOT" > /dev/null 2>&1; then
    echo "ERROR: Build failed"
    exit 1
fi
echo "✓ Build successful"

# Check if binary exists
if [ ! -f "$PROJECT_ROOT/bzip2_impl" ]; then
    echo "ERROR: Binary not created"
    exit 1
fi
echo "✓ Binary created"

# Try basic compression test
echo "Testing basic compression..."
echo "test" > "$PROJECT_ROOT/test_input.txt"
if ! "$PROJECT_ROOT/bzip2_impl" compress "$PROJECT_ROOT/test_input.txt" "$PROJECT_ROOT/test.bz" > /dev/null 2>&1; then
    echo "ERROR: Compression failed"
    rm -f "$PROJECT_ROOT/test_input.txt"
    exit 1
fi
echo "✓ Compression works"

# Try basic decompression test
if ! "$PROJECT_ROOT/bzip2_impl" decompress "$PROJECT_ROOT/test.bz" "$PROJECT_ROOT/test_output.txt" > /dev/null 2>&1; then
    echo "ERROR: Decompression failed"
    rm -f "$PROJECT_ROOT/test_input.txt" "$PROJECT_ROOT/test.bz"
    exit 1
fi
echo "✓ Decompression works"

# Check roundtrip
if ! cmp -s "$PROJECT_ROOT/test_input.txt" "$PROJECT_ROOT/test_output.txt"; then
    echo "ERROR: Roundtrip failed"
    rm -f "$PROJECT_ROOT/test_input.txt" "$PROJECT_ROOT/test.bz" "$PROJECT_ROOT/test_output.txt"
    exit 1
fi
echo "✓ Roundtrip successful"

# Cleanup
rm -f "$PROJECT_ROOT/test_input.txt" "$PROJECT_ROOT/test.bz" "$PROJECT_ROOT/test_output.txt"

echo ""
echo "✓ All validation checks passed!"
echo "The BZip2 implementation is working correctly."