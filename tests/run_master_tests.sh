#!/bin/bash
# Master test runner - runs all test suites

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TEST_DIR="$PROJECT_ROOT/tests"

echo "=========================================="
echo "BZip2 Comprehensive Test Suite"
echo "=========================================="
echo ""

# Check if binary exists
if [ ! -f "$PROJECT_ROOT/bzip2_impl" ]; then
    echo "Error: bzip2_impl binary not found. Please build the project first."
    echo "Run: make"
    exit 1
fi

# Run unit tests
echo "1. Running Unit Tests..."
if [ -f "$TEST_DIR/run_unit_tests.sh" ]; then
    bash "$TEST_DIR/run_unit_tests.sh"
else
    echo "Unit test script not found, skipping..."
fi
echo ""

# Run integration tests
echo "2. Running Integration Tests..."
if [ -f "$TEST_DIR/run_all_tests.sh" ]; then
    bash "$TEST_DIR/run_all_tests.sh"
else
    echo "Integration test script not found, skipping..."
fi
echo ""

# Run performance tests
echo "3. Running Performance Tests..."
if [ -f "$TEST_DIR/run_performance_tests.sh" ]; then
    bash "$TEST_DIR/run_performance_tests.sh"
else
    echo "Performance test script not found, skipping..."
fi
echo ""

# Generate summary
echo "4. Generating Test Summary..."
if [ -f "$TEST_DIR/generate_summary.sh" ]; then
    bash "$TEST_DIR/generate_summary.sh"
else
    echo "Summary script not found, skipping..."
fi

echo ""
echo "=========================================="
echo "All tests completed!"
echo "=========================================="