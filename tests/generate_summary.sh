#!/bin/bash
# Test summary generator

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG_FILE="$PROJECT_ROOT/tests/test_results.log"
RESULTS_DIR="$PROJECT_ROOT/tests/results"

if [ ! -f "$LOG_FILE" ]; then
    echo "No test results found. Run tests first."
    exit 1
fi

echo "=========================================="
echo "BZip2 Test Summary"
echo "=========================================="
echo ""

# Count results
total_tests=$(grep -c "^PASS:\|^FAIL:\|^SKIP:" "$LOG_FILE")
passed_tests=$(grep -c "^PASS:" "$LOG_FILE")
failed_tests=$(grep -c "^FAIL:" "$LOG_FILE")
skipped_tests=$(grep -c "^SKIP:" "$LOG_FILE")

echo "Total Tests: $total_tests"
echo "Passed: $passed_tests"
echo "Failed: $failed_tests"
echo "Skipped: $skipped_tests"
echo ""

if [ "$total_tests" -gt 0 ]; then
    success_rate=$((passed_tests * 100 / total_tests))
    echo "Success Rate: ${success_rate}%"
fi

echo ""
echo "Detailed Results:"
echo "----------------------------------------"

# Show failed tests
if [ "$failed_tests" -gt 0 ]; then
    echo "FAILED TESTS:"
    grep "^FAIL:" "$LOG_FILE" | sed 's/^FAIL: //' | while read -r line; do
        echo "  - $line"
    done
    echo ""
fi

# Show skipped tests
if [ "$skipped_tests" -gt 0 ]; then
    echo "SKIPPED TESTS:"
    grep "^SKIP:" "$LOG_FILE" | sed 's/^SKIP: //' | while read -r line; do
        echo "  - $line"
    done
    echo ""
fi

echo "Full log available at: $LOG_FILE"

# Performance summary if available
PERF_FILE="$RESULTS_DIR/performance.csv"
if [ -f "$PERF_FILE" ]; then
    echo ""
    echo "Performance Summary:"
    echo "----------------------------------------"
    echo "File,Original,Compressed,Ratio,Comp Time,Decomp Time,Comp Speed,Decomp Speed,Status"
    tail -n +2 "$PERF_FILE" | head -5  # Show first 5 results
    if [ $(wc -l < "$PERF_FILE") -gt 6 ]; then
        echo "... ($(($(wc -l < "$PERF_FILE") - 1)) total benchmark files tested)"
    fi
fi