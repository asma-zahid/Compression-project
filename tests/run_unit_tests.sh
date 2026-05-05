#!/bin/bash
# Unit test runner for BZip2 components

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC_DIR="$PROJECT_ROOT/src"
INCLUDE_DIR="$PROJECT_ROOT/include"
TEST_DIR="$PROJECT_ROOT/tests"

# Compile unit tests
echo "Compiling unit tests..."
gcc -Wall -Wextra -O2 -std=c99 \
    -I"$INCLUDE_DIR" \
    "$TEST_DIR/unit_tests.c" \
    "$SRC_DIR/rle.c" \
    "$SRC_DIR/bwt.c" \
    "$SRC_DIR/mtf.c" \
    "$SRC_DIR/huffman.c" \
    "$SRC_DIR/config.c" \
    "$SRC_DIR/block.c" \
    -o "$TEST_DIR/unit_tests"

# Run unit tests
echo "Running unit tests..."
"$TEST_DIR/unit_tests"

echo "Unit tests completed successfully!"