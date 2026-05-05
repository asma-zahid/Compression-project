#!/bin/bash
# Performance test script for BZip2 implementation

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$PROJECT_ROOT/bzip2_impl"
BENCHMARK_DIR="$PROJECT_ROOT/benchmarks"
RESULTS_DIR="$PROJECT_ROOT/tests/results"
TEMP_DIR="$PROJECT_ROOT/tests/temp"

mkdir -p "$RESULTS_DIR" "$TEMP_DIR"

echo "=========================================="
echo "BZip2 Performance Test"
echo "=========================================="
echo ""

# Test each benchmark file
for input_file in "$BENCHMARK_DIR"/*; do
    if [ -f "$input_file" ]; then
        filename=$(basename "$input_file")
        compressed_file="$TEMP_DIR/${filename}.bz"
        output_file="$TEMP_DIR/${filename}_out"

        echo "Testing: $filename"

        # Get original size
        orig_size=$(stat -c %s "$input_file")

        # Compress with timing
        start_time=$(date +%s.%3N)
        "$BIN" compress "$input_file" "$compressed_file" > /dev/null 2>&1
        end_time=$(date +%s.%3N)
        comp_time=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "0")

        comp_size=$(stat -c %s "$compressed_file")
        ratio=$(echo "scale=3; $orig_size / $comp_size" | bc 2>/dev/null || echo "N/A")

        # Decompress with timing
        start_time=$(date +%s.%3N)
        "$BIN" decompress "$compressed_file" "$output_file" > /dev/null 2>&1
        end_time=$(date +%s.%3N)
        decomp_time=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "0")

        # Verify correctness
        if cmp -s "$input_file" "$output_file"; then
            status="PASS"
        else
            status="FAIL"
        fi

        # Calculate speeds
        comp_speed=$(echo "scale=2; $orig_size / 1024 / 1024 / $comp_time" | bc 2>/dev/null || echo "N/A")
        decomp_speed=$(echo "scale=2; $orig_size / 1024 / 1024 / $decomp_time" | bc 2>/dev/null || echo "N/A")

        echo "  Status: $status"
        echo "  Original: ${orig_size} bytes"
        echo "  Compressed: ${comp_size} bytes"
        echo "  Ratio: ${ratio}x"
        echo "  Compress time: ${comp_time}s (${comp_speed} MB/s)"
        echo "  Decompress time: ${decomp_time}s (${decomp_speed} MB/s)"
        echo ""

        # Log results
        echo "$filename,$orig_size,$comp_size,$ratio,$comp_time,$decomp_time,$comp_speed,$decomp_speed,$status" >> "$RESULTS_DIR/performance.csv"
    fi
done

# Generate summary
echo "Performance test completed."
echo "Results saved to: $RESULTS_DIR/performance.csv"

# Cleanup
rm -rf "$TEMP_DIR"