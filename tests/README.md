# Test Cases for BZip2 Implementation

This folder contains comprehensive test cases for the BZip2 compression algorithm implementation.

## Prerequisites

- **Linux/WSL**: Bash scripts work directly
- **Windows**: Use PowerShell scripts (`.ps1` versions) or install MSYS2/WSL

## Test Categories

1. **Unit Tests**: Test individual components (RLE, BWT, MTF, Huffman)
2. **Integration Tests**: Test the full compression/decompression pipeline
3. **Edge Cases**: Test boundary conditions and special inputs
4. **Performance Tests**: Test with large files and benchmark performance
5. **Regression Tests**: Ensure fixes don't break existing functionality

## Running Tests

**Note**: The `bzip2_impl` binary is a Linux ELF executable. On Windows, you must:
- Use WSL (Windows Subsystem for Linux)
- Or build with MinGW/MSYS2
- Or run tests in a Linux environment

Use the provided scripts to run different test suites:

- `run_unit_tests.sh` / `run_unit_tests.ps1`: Run unit tests for individual components
- `run_all_tests.sh` / `run_all_tests.ps1`: Run full pipeline roundtrip tests
- `run_performance_tests.sh` / `run_performance_tests.ps1`: Run performance benchmarks
- `run_master_tests.sh` / `run_master_tests.ps1`: Run all tests in sequence
- `validate_build.sh` / `validate_build.ps1`: Quick build validation
- `generate_summary.sh` / `generate_summary.ps1`: Generate results summary

## Test Data

- `test_data/`: Directory containing various test input files
  - `single_char.txt`: Single character file
  - `empty_file.txt`: Empty file
  - `all_zeros_1000.bin`: 1000 zero bytes
  - `pangram.txt`: Standard pangram text
  - `repeated_chars.txt`: Highly repetitive text

## Test Results

Test results are logged to `test_results.log` with PASS/FAIL status for each test.
Performance results are saved to `results/performance.csv`.

## Directory Structure

```
tests/
├── README.md                    # This file
├── test_data/                   # Test input files
├── results/                     # Test output and logs
├── temp/                        # Temporary files (created during tests)
├── unit_tests.c                 # Unit test source code
├── run_unit_tests.sh           # Unit test runner
├── run_all_tests.sh            # Integration test runner
├── run_performance_tests.sh    # Performance test runner
├── run_master_tests.sh         # Master test runner
└── generate_summary.sh         # Results summary generator
```