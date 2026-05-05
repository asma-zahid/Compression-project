#!/bin/bash
# Comprehensive test suite for BZip2 implementation

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$PROJECT_ROOT/bzip2_impl"
TEST_DATA_DIR="$PROJECT_ROOT/tests/test_data"
TEMP_DIR="$PROJECT_ROOT/tests/temp"
LOG_FILE="$PROJECT_ROOT/tests/test_results.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create temp directory
mkdir -p "$TEMP_DIR"

# Logging function
log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $*" | tee -a "$LOG_FILE"
}

# Test result function
test_result() {
    local test_name="$1"
    local result="$2"
    local details="$3"

    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}[PASS]${NC} $test_name"
        echo "PASS: $test_name - $details" >> "$LOG_FILE"
    elif [ "$result" = "FAIL" ]; then
        echo -e "${RED}[FAIL]${NC} $test_name"
        echo "FAIL: $test_name - $details" >> "$LOG_FILE"
    else
        echo -e "${YELLOW}[SKIP]${NC} $test_name"
        echo "SKIP: $test_name - $details" >> "$LOG_FILE"
    fi
}

# Roundtrip test function
run_roundtrip_test() {
    local test_name="$1"
    local input_file="$2"
    local compressed_file="$TEMP_DIR/$(basename "$input_file" .txt).bz"
    local output_file="$TEMP_DIR/$(basename "$input_file" .txt)_out.txt"

    if [ ! -f "$input_file" ]; then
        test_result "$test_name" "SKIP" "Input file $input_file not found"
        return
    fi

    # Compress
    if ! "$BIN" compress "$input_file" "$compressed_file" > /dev/null 2>&1; then
        test_result "$test_name" "FAIL" "Compression failed"
        return
    fi

    # Decompress
    if ! "$BIN" decompress "$compressed_file" "$output_file" > /dev/null 2>&1; then
        test_result "$test_name" "FAIL" "Decompression failed"
        return
    fi

    # Compare
    if cmp -s "$input_file" "$output_file"; then
        local orig_size=$(stat -c %s "$input_file")
        local comp_size=$(stat -c %s "$compressed_file")
        local ratio=$(python3 -c "print($orig_size/$comp_size)" 2>/dev/null || echo "N/A")
        test_result "$test_name" "PASS" "Roundtrip OK, ratio: ${ratio}x"
    else
        test_result "$test_name" "FAIL" "Roundtrip failed - files differ"
    fi

    # Cleanup
    rm -f "$compressed_file" "$output_file"
}

# Main test execution
echo "==========================================" | tee "$LOG_FILE"
echo "BZip2 Implementation Test Suite" | tee -a "$LOG_FILE"
echo "==========================================" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

log "Starting test execution..."

# Basic functionality tests
echo "Running basic functionality tests..."
run_roundtrip_test "Single Character" "$TEST_DATA_DIR/single_char.txt"
run_roundtrip_test "Empty File" "$TEST_DATA_DIR/empty_file.txt"
run_roundtrip_test "All Zeros (1000 bytes)" "$TEST_DATA_DIR/all_zeros_1000.bin"
run_roundtrip_test "Pangram" "$TEST_DATA_DIR/pangram.txt"
run_roundtrip_test "Repeated Characters" "$TEST_DATA_DIR/repeated_chars.txt"

# Benchmark tests (using existing benchmark files)
echo ""
echo "Running benchmark tests..."
BENCHMARK_DIR="$PROJECT_ROOT/benchmarks"
if [ -d "$BENCHMARK_DIR" ]; then
    for file in "$BENCHMARK_DIR"/*; do
        if [ -f "$file" ]; then
            run_roundtrip_test "Benchmark: $(basename "$file")" "$file"
        fi
    done
else
    log "Benchmark directory not found, skipping benchmark tests"
fi

# Edge case tests
echo ""
echo "Running edge case tests..."

# Very small files
echo -n "x" > "$TEMP_DIR/tiny.txt"
run_roundtrip_test "Tiny File (1 byte)" "$TEMP_DIR/tiny.txt"

# Large repetitive file
python3 -c "
import sys
with open('$TEMP_DIR/large_repetitive.txt', 'w') as f:
    for i in range(1000):
        f.write('ABCDEFGH' * 100)
" 2>/dev/null || echo "ABCDEFGH" > "$TEMP_DIR/large_repetitive.txt"
run_roundtrip_test "Large Repetitive" "$TEMP_DIR/large_repetitive.txt"

# Binary data
python3 -c "
import sys
with open('$TEMP_DIR/binary_data.bin', 'wb') as f:
    f.write(bytes(range(256)) * 10)
" 2>/dev/null || echo "Binary data test" > "$TEMP_DIR/binary_data.bin"
run_roundtrip_test "Binary Data" "$TEMP_DIR/binary_data.bin"

# Cleanup temp files
rm -rf "$TEMP_DIR"

echo ""
echo "==========================================" | tee -a "$LOG_FILE"
echo "Test execution completed" | tee -a "$LOG_FILE"
echo "Results logged to: $LOG_FILE" | tee -a "$LOG_FILE"
echo "==========================================" | tee -a "$LOG_FILE"