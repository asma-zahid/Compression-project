# BZip2 Compression Algorithm – Implementation

> **Course:** Data Compression  
> **Instructor:** Dr. Faisal Aslam  
> **Team Size:** 3 members  
> **Language:** C (C99 standard)  
> **Target Platform:** Linux / Ubuntu (VirtualBox)

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Repository Structure](#2-repository-structure)
3. [System Architecture](#3-system-architecture)
4. [Build Instructions](#4-build-instructions)
5. [Usage](#5-usage)
6. [Configuration](#6-configuration)
7. [Stage 1 – Block Division, RLE-1, BWT](#7-stage-1--block-division-rle-1-bwt) ✅
8. [Stage 2 – MTF, RLE-2](#8-stage-2--mtf-rle-2) 🔲
9. [Stage 3 – Canonical Huffman Coding](#9-stage-3--canonical-huffman-coding) 🔲
10. [Compressed File Format](#10-compressed-file-format)
11. [Performance Results](#11-performance-results)
12. [Extra Features](#12-extra-features)
13. [Team Contributions](#13-team-contributions)
14. [References](#14-references)

---

## 1. Project Overview

This project implements a simplified version of the **BZip2 lossless compression algorithm** from scratch in C. BZip2 is a high-quality compressor that combines several well-known algorithms in a pipeline to achieve strong compression ratios, especially on text data.

### Full Compression Pipeline

```
Input File
    │
    ▼
Block Division          ← splits large files into manageable chunks
    │
    ▼
RLE-1 Encoding          ← Stage 1: removes obvious runs before BWT
    │
    ▼
Burrows-Wheeler         ← Stage 1: groups similar characters together
Transform (BWT)
    │
    ▼
Move-to-Front           ← Stage 2: converts BWT output to small integers
(MTF)
    │
    ▼
RLE-2 Encoding          ← Stage 2: compresses the many zeros MTF produces
    │
    ▼
Canonical Huffman       ← Stage 3: entropy coding for final bit reduction
Coding
    │
    ▼
Compressed Output
```

### Project Timeline

| Stage | Components | Weight | Status |
|-------|-----------|--------|--------|
| Stage 1 | Block Division + RLE-1 + BWT | 50% | ✅ Complete |
| Stage 2 | MTF + RLE-2 | 25% | 🔲 Pending |
| Stage 3 | Canonical Huffman Coding | 25% | 🔲 Pending |
| Extra | Suffix Array BWT, Enhanced RLE, ANS | +10% | 🔲 Pending |

---

## 2. Repository Structure

```
project-bzip2/
│
├── src/
│   ├── main.c          # Entry point – CLI, compress/decompress pipeline
│   ├── block.c         # Block division and reassembly
│   ├── rle.c           # RLE-1 (Stage 1) and RLE-2 stub (Stage 2)
│   ├── bwt.c           # Burrows-Wheeler Transform (Stage 1)
│   ├── mtf.c           # Move-to-Front + RLE-2 stubs (Stage 2)
│   ├── huffman.c       # Canonical Huffman coding stubs (Stage 3)
│   └── config.c        # INI config file parser
│
├── include/
│   └── bzip2.h         # All structs, typedefs, and function prototypes
│
├── benchmarks/         # Place input test files here
│   └── (test files)
│
├── results/            # Compressed / decompressed output files
│   └── (output files)
│
├── obj/                # Object files (auto-created by make)
│
├── Makefile            # Cross-platform build system
├── config.ini          # Runtime configuration
└── README.md           # This file
```

---

## 3. System Architecture

### Data Structures

```c
/* A single block of data */
typedef struct {
    unsigned char *data;        // Pointer to block data
    size_t         size;        // Current size of block
    size_t         original_size; // Original size before compression
} Block;

/* Manages all blocks of a file */
typedef struct {
    Block  *blocks;             // Array of blocks
    int     num_blocks;         // Number of blocks
    size_t  block_size;         // Configurable block size
} BlockManager;

/* BWT rotation descriptor */
typedef struct {
    char *rotation;             // Rotation string (for spec compliance)
    int   index;                // Original index
} Rotation;

/* Huffman code (Stage 3) */
typedef struct {
    unsigned short code;        // Huffman code bits
    unsigned char  length;      // Code length in bits
} HuffmanCode;

/* Huffman tree node (Stage 3) */
typedef struct Node {
    unsigned char  symbol;      // Byte value 0–255
    int            freq;        // Frequency count
    struct Node   *left;
    struct Node   *right;
} HuffmanNode;
```

---

## 4. Build Instructions

### Prerequisites

```bash
sudo apt update
sudo apt install gcc make -y
```

For Windows cross-compilation (optional):
```bash
sudo apt install mingw-w64 -y
```

### Build

```bash
cd "Compression project"
make
```

This creates the `bzip2_impl` binary and the `obj/`, `results/`, `benchmarks/` directories automatically.

### Other Targets

| Command | Description |
|---------|-------------|
| `make` | Build everything |
| `make test` | Build + run a quick roundtrip correctness test |
| `make clean` | Remove binary and object files |
| `make windows` | Cross-compile for Windows (`bzip2_impl.exe`) |

### Expected Build Output

```
mkdir -p obj results benchmarks
gcc -Wall -Wextra -O2 -std=c99 -Iinclude -c src/main.c    -o obj/main.o
gcc -Wall -Wextra -O2 -std=c99 -Iinclude -c src/rle.c     -o obj/rle.o
gcc -Wall -Wextra -O2 -std=c99 -Iinclude -c src/bwt.c     -o obj/bwt.o
gcc -Wall -Wextra -O2 -std=c99 -Iinclude -c src/block.c   -o obj/block.o
gcc -Wall -Wextra -O2 -std=c99 -Iinclude -c src/mtf.c     -o obj/mtf.o
gcc -Wall -Wextra -O2 -std=c99 -Iinclude -c src/huffman.c -o obj/huffman.o
gcc -Wall -Wextra -O2 -std=c99 -Iinclude -c src/config.c  -o obj/config.o
gcc -Wall -Wextra -O2 -std=c99 -o bzip2_impl obj/*.o

Build successful -> bzip2_impl
```

---

## 5. Usage

### Compress a file

```bash
./bzip2_impl compress <input_file> <output_file>
```

Example:
```bash
./bzip2_impl compress benchmarks/sample.txt results/sample.bz
```

### Decompress a file

```bash
./bzip2_impl decompress <compressed_file> <output_file>
```

Example:
```bash
./bzip2_impl decompress results/sample.bz results/sample_recovered.txt
```

### Verify correctness

```bash
diff benchmarks/sample.txt results/sample_recovered.txt
echo $?   # 0 = identical (correct), non-zero = error
```

### Run the built-in test

```bash
make test
```

Expected:
```
--- Running self-test ---
  block   0: orig=     78  rle1=     52  ratio=1.500  pidx=23
Compression done: 78 → 73 bytes  (ratio 1.068)  0.001 s
Decompression done: 78 bytes  0.001 s

*** TEST PASSED – roundtrip OK ***
```

---

## 6. Configuration

The file `config.ini` controls runtime behaviour. Edit it before running.

```ini
[General]
block_size = 500000      # Block size in bytes (100KB – 900KB)
rle1_enabled = true      # Enable/disable RLE-1 pass
bwt_type = matrix        # BWT algorithm: matrix or suffix_array
mtf_enabled = true       # Stage 2
rle2_enabled = true      # Stage 2
huffman_enabled = true   # Stage 3

[Performance]
benchmark_mode = false   # Run full benchmark suite
output_metrics = true    # Print timing and ratio info

[Paths]
input_directory = ./benchmarks/
output_directory = ./results/
```

### Configuration Options Explained

| Key | Values | Effect |
|-----|--------|--------|
| `block_size` | 102400 – 921600 | Larger = better compression, more memory and time |
| `rle1_enabled` | true / false | Disable to skip RLE-1 (useful for testing BWT in isolation) |
| `bwt_type` | matrix | Only `matrix` supported in Stage 1. `suffix_array` planned for extra credit |
| `output_metrics` | true / false | Toggle verbose output of ratios and timings |

---

## 7. Stage 1 – Block Division, RLE-1, BWT

**Status: ✅ Complete**  
**Grade weight: 50%**

### 7.1 Block Division (`src/block.c`)

Large input files are split into independent blocks of configurable size so that:
- Memory usage stays bounded regardless of file size
- Each block can be processed independently

**Key functions:**

```c
BlockManager *divide_into_blocks(const char *filename, size_t block_size);
int           reassemble_blocks(BlockManager *manager, const char *output_filename);
void          free_block_manager(BlockManager *manager);
```

**How it works:**
1. Open file, determine total size with `fseek`/`ftell`
2. Compute `num_blocks = ceil(file_size / block_size)`
3. Read each chunk into a `Block` struct
4. Return a `BlockManager` holding all blocks

### 7.2 RLE-1 (`src/rle.c`)

Run-Length Encoding (first pass) reduces runs of identical bytes before the BWT.

**Encoding format:** Each run is stored as a `(count, byte)` pair.

```
Input  : A  B  B  B  C  C  C  C  D
Runs   : 1A    3B       4C       1D
Encoded: 01 41  03 42  04 43  01 44   (hex: count then ASCII value)
```

- Count is 1 byte (1–255). Runs longer than 255 are split automatically.
- Worst case output = 2× input (all unique bytes — no runs).
- Best case output ≪ input (all identical bytes).

**Key functions:**

```c
void rle1_encode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len);
void rle1_decode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len);
```

**Decode is the exact inverse:** read `(count, byte)` pairs and expand.

### 7.3 Burrows-Wheeler Transform (`src/bwt.c`)

The BWT rearranges bytes so that similar characters cluster together, making subsequent compression stages far more effective.

**Forward transform (matrix method):**

1. Conceptually form all `n` cyclic rotations of the block.
2. Sort them lexicographically.
3. The **last column** of the sorted matrix is the BWT output.
4. Record the **primary index** — the row that corresponds to the original string.

```
Input: BANANA   (n = 6)

All cyclic rotations sorted:
  Row 0:  A B A N A N   → last char: N
  Row 1:  A N A B A N   → last char: N
  Row 2:  A N A N A B   → last char: B
  Row 3:  B A N A N A   → last char: A   ← original (primary_index = 3)
  Row 4:  N A B A N A   → last char: A
  Row 5:  N A N A B A   → last char: A

BWT output : N N B A A A
Primary idx: 3
```

**Inverse transform (LF-mapping):**

Given `L` (BWT output) and `primary_index`:

1. Compute `C[c]` = number of symbols in `L` strictly less than `c`.
2. Build `LF[i] = C[L[i]] + rank(L[i], i)` — maps last column positions to first column positions.
3. Walk the LF chain backwards from `primary_index` to recover the original string.

Verified with the BANANA example: starting from `primary_index = 3` with `L = "NNBAAA"` correctly recovers `"BANANA"`.

**Key functions:**

```c
int  compare_rotations(const void *a, const void *b);  // used by qsort
void bwt_encode(unsigned char *input, size_t len,
                unsigned char *output, int *primary_index);
void bwt_decode(unsigned char *input, size_t len,
                int primary_index, unsigned char *output);
```

**Complexity note:** The matrix BWT uses `qsort` with an O(n) comparator → O(n² log n) total. This is the correct "matrix" approach specified. For large blocks (> 50 KB) it will be slow. The suffix array method (O(n log n)) is planned as extra credit.

### 7.4 Stage 1 Evaluation Checklist

- [x] RLE-1 encode produces correct output
- [x] RLE-1 decode is exact inverse of encode
- [x] BWT forward transform verified with BANANA example
- [x] BWT inverse transform recovers original exactly
- [x] Block division handles files larger than one block
- [x] Block reassembly produces byte-identical output
- [x] Config file parsed correctly with inline comments
- [x] Roundtrip test passes (`make test`)

---

## 8. Stage 2 – MTF, RLE-2

**Status: 🔲 Not yet implemented**  
**Grade weight: 25%**

> _This section will be completed and updated when Stage 2 is implemented._

### 8.1 Move-to-Front Transform (`src/mtf.c`)

MTF encodes each byte as its current position in a maintained symbol list, then moves that symbol to the front. Because BWT clusters similar bytes together, MTF produces many small values (especially zeros).

**Algorithm (to be implemented):**

```
Initialize list: [0, 1, 2, ..., 255]

For each input byte b:
  1. Find index i of b in the list
  2. Output i
  3. Move b to front of list
```

**Function prototypes (already declared):**

```c
void mtf_encode(unsigned char *input, size_t len, unsigned char *output);
void mtf_decode(unsigned char *input, size_t len, unsigned char *output);
```

### 8.2 RLE-2 (`src/mtf.c`)

RLE-2 is a specialized run-length encoder targeting the MTF output stream, which contains long runs of zeros. It uses a different encoding scheme than RLE-1, representing zero-runs efficiently with two special symbols (RUNA / RUNB).

**Function prototypes (already declared):**

```c
void rle2_encode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len);
void rle2_decode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len);
```

### 8.3 Stage 2 Evaluation Checklist

- [ ] MTF encode produces correct indices
- [ ] MTF decode is exact inverse
- [ ] RLE-2 encode correctly targets zero-runs
- [ ] RLE-2 decode is exact inverse
- [ ] Full pipeline RLE-1 → BWT → MTF → RLE-2 roundtrip passes

---

## 9. Stage 3 – Canonical Huffman Coding

**Status: 🔲 Not yet implemented**  
**Grade weight: 25%**

> _This section will be completed and updated when Stage 3 is implemented._

### 9.1 Canonical Huffman Coding (`src/huffman.c`)

Canonical Huffman assigns variable-length prefix codes to symbols based on frequency. The "canonical" form requires storing only code lengths (not the full tree), making the compressed header very compact.

**Steps (to be implemented):**

1. Count symbol frequencies in the input.
2. Build a min-heap-based Huffman tree (`build_huffman_tree`).
3. Assign code lengths by traversing the tree.
4. Convert to canonical form — sort by (length, symbol), assign codes in order.
5. Write header: 256 code lengths (1 byte each).
6. Write encoded data bit by bit.

**Function prototypes (already declared):**

```c
void build_huffman_tree(int *frequencies, HuffmanNode **root);
void generate_canonical_codes(HuffmanNode *root, HuffmanCode *codes);
void huffman_encode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len);
void huffman_decode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len);
void write_header(HuffmanCode *codes, unsigned char *output, size_t *out_len);
void encode_data(unsigned char *input, size_t len,
                 HuffmanCode *codes, unsigned char *output, size_t *out_len);
```

### 9.2 Stage 3 Evaluation Checklist

- [ ] Huffman tree built correctly from frequencies
- [ ] Canonical codes generated correctly
- [ ] Header written and read back correctly
- [ ] Huffman encode/decode roundtrip passes
- [ ] Full pipeline RLE-1 → BWT → MTF → RLE-2 → Huffman roundtrip passes

---

## 10. Compressed File Format

### File Header (13 bytes)

| Offset | Size | Type | Field |
|--------|------|------|-------|
| 0 | 4 | char[4] | Magic: `"MYBZ"` |
| 4 | 1 | uint8 | Stage version (1 = Stage 1) |
| 5 | 4 | uint32 LE | Block size used during compression |
| 9 | 4 | uint32 LE | Number of blocks |

### Per-Block Record (12 bytes header + data)

| Offset | Size | Type | Field |
|--------|------|------|-------|
| 0 | 4 | uint32 LE | Original block size (bytes) |
| 4 | 4 | uint32 LE | Encoded payload size (bytes) |
| 8 | 4 | int32 LE | BWT primary index |
| 12 | N | bytes | BWT-encoded payload (N = encoded size) |

> The format will be extended in Stage 2 and Stage 3 to include MTF/RLE-2/Huffman headers per block.

---

## 11. Performance Results

> _Results will be added here after benchmarking on the standard corpora._

### Benchmark Datasets

| Dataset | Size | Type |
|---------|------|------|
| Canterbury Corpus | Various | Mixed |
| Calgary Corpus | Various | Mixed |
| Silesia Corpus | Various | Mixed |
| Large Text Files | 10 MB – 100 MB | Text |
| Binary Files | 10 MB – 100 MB | Binary |

### Metrics Formula

```
Score = w1 × (C_ref / C) + w2 × (S / S_ref)
```

Where:
- `C` = compression ratio achieved
- `C_ref` = reference ratio (system bzip2)
- `S` = speed in MB/s
- `S_ref` = reference speed (system bzip2)
- `w1`, `w2` = weights set by instructor

### Stage 1 Results (Preliminary)

| File | Original Size | Compressed Size | Ratio | Time |
|------|-------------|----------------|-------|------|
| _To be measured_ | – | – | – | – |

> Full results table and graphs will be added here after Stage 3 is complete.

---

## 12. Extra Features

> _To be implemented after Stage 3._

### 12.1 Suffix Array BWT (Extra Credit)

Replaces the O(n² log n) matrix sort with an O(n log n) suffix array construction, making large block sizes (500 KB – 900 KB) practical.

```c
int *build_suffix_array(unsigned char *text, int n);
```

Enable via `config.ini`:
```ini
bwt_type = suffix_array
```

### 12.2 Enhanced RLE (Extra Credit)

- **Threshold RLE:** Only encode runs longer than a configurable minimum length.
- **Adaptive RLE:** Dynamically skip encoding when it would expand the data.

### 12.3 Alternative Entropy Coding (Extra Credit)

- **ANS (Asymmetric Numeral Systems):** Near-optimal entropy coder, faster than Huffman.
- **Range Coding:** Arithmetic coding with integer arithmetic.

---

## 13. Team Contributions

### Stage 1

| Member | Student ID | Component | Files |
|--------|-----------|-----------|-------|
| Muhammad Haris | 22L-6972 | Burrows-Wheeler Transform (BWT) — forward matrix sort + inverse LF-mapping | `src/bwt.c` |
| Asma Zahid | 22L-6718 | RLE-1 — run-length encode/decode | `src/rle.c` |
| Osamah Ashraf | 22L-6816 | Block Division & File Handling — block split, reassembly, config parser | `src/block.c`, `src/config.c` |

### Stage 2 _(to be updated)_

| Member | Student ID | Component | Files |
|--------|-----------|-----------|-------|
| – | – | – | – |

### Stage 3 _(to be updated)_

| Member | Student ID | Component | Files |
|--------|-----------|-----------|-------|
| – | – | – | – |

---

## 14. References

1. Seward, J. (1996). *The bzip2 and libbzip2 official home page.*
2. Burrows, M., & Wheeler, D. J. (1994). *A block-sorting lossless data compression algorithm.* DEC SRC Research Report 124.
3. Huffman, D. A. (1952). *A method for the construction of minimum-redundancy codes.* Proceedings of the IRE, 40(9), 1098–1101.
4. Manber, U., & Myers, G. (1993). *Suffix arrays: a new method for on-line string searches.* SIAM Journal on Computing, 22(5), 935–948.
5. Nelson, M., & Gailly, J. (1995). *The Data Compression Book.* M&T Books.
