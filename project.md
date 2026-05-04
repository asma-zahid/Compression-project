Data Compression Course Project Implementation of BZip2 Compression Algorithm 

Course Instructor: Dr. Faisal Aslam 

Date: April 14, 2026  
Data Compression Project BZip2 Implementation Contents 

1 Introduction 3 1.1 Project Overview . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 3 1.2 Project Timeline . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 3 1.3 Evaluation Criteria . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 3 

2 System Architecture 4 2.1 Overall Block Diagram . . . . . . . . . . . . . . . . . . . . . . . . . . . . 4 2.2 Configuration Management . . . . . . . . . . . . . . . . . . . . . . . . . . 4 

3 Stage 1 Implementation (1.5 Weeks) 5 3.1 Block Division and File Handling . . . . . . . . . . . . . . . . . . . . . . 5 3.1.1 Requirements . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 5 3.1.2 Data Structures . . . . . . . . . . . . . . . . . . . . . . . . . . . . 5 3.1.3 Function Prototypes . . . . . . . . . . . . . . . . . . . . . . . . . 5 3.2 RLE-1 Implementation . . . . . . . . . . . . . . . . . . . . . . . . . . . . 6 3.2.1 Description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 6 3.2.2 Function Prototypes . . . . . . . . . . . . . . . . . . . . . . . . . 6 3.3 Burrows-Wheeler Transform (BWT) . . . . . . . . . . . . . . . . . . . . 6 3.3.1 Description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 6 3.3.2 Data Structures . . . . . . . . . . . . . . . . . . . . . . . . . . . . 7 3.3.3 Function Prototypes . . . . . . . . . . . . . . . . . . . . . . . . . 7 3.4 Stage 1 Evaluation . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 7 

4 Stage 2 Implementation (6 Days) 8 4.1 Move-to-Front (MTF) Transform . . . . . . . . . . . . . . . . . . . . . . 8 4.1.1 Description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 8 4.1.2 Function Prototypes . . . . . . . . . . . . . . . . . . . . . . . . . 8 4.2 RLE-2 Implementation . . . . . . . . . . . . . . . . . . . . . . . . . . . . 8 4.2.1 Description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 8 4.2.2 Function Prototypes . . . . . . . . . . . . . . . . . . . . . . . . . 8 4.3 Stage 2 Evaluation . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 9 

5 Stage 3 Implementation (1 Week) 9 5.1 Canonical Huffman Coding . . . . . . . . . . . . . . . . . . . . . . . . . . 9 5.1.1 Description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 9 5.1.2 Data Structures . . . . . . . . . . . . . . . . . . . . . . . . . . . . 9 5.1.3 Function Prototypes . . . . . . . . . . . . . . . . . . . . . . . . . 9 

6 Build System and Platform Support 11 6.1 Makefile Requirements . . . . . . . . . . . . . . . . . . . . . . . . . . . . 11 

7 Performance Evaluation and Benchmarking 11 7.1 Benchmark Datasets . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 11 7.2 Performance Metrics . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 11 7.3 Automated Results Generation . . . . . . . . . . . . . . . . . . . . . . . 12 

2  
Data Compression Project BZip2 Implementation 

8 Extra Features and Enhancements (10% Marks) 12 8.1 Enhanced RLE Versions (Extra Marks) . . . . . . . . . . . . . . . . . . . 12 8.2 Suffix Array-based BWT (Extra Marks) . . . . . . . . . . . . . . . . . . 12 8.3 Alternative Entropy Coding . . . . . . . . . . . . . . . . . . . . . . . . . 12 

9 Submission Guidelines 13 9.1 GitHub Repository Structure . . . . . . . . . . . . . . . . . . . . . . . . 13 9.2 README.md Requirements . . . . . . . . . . . . . . . . . . . . . . . . . 13 9.3 Graph Generation . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 13 

10 Important Notes for Students 14 11 Conclusion 14 

3  
Data Compression Project BZip2 Implementation 

1 Introduction 

1.1 Project Overview 

This project involves implementing a simplified version of the BZip2 compression algo rithm. The project spans three weeks and is completed by a team of three individu als. The implementation includes all major components of BZip2: Run-Length Encoding (RLE), Burrows-Wheeler Transform (BWT), Move-to-Front (MTF) transform, and Huff man coding. 

Important: Students must write their own code. Only function prototypes and struct definitions are provided as guidance. No implementation code is given. 

1.2 Project Timeline 

• Total Duration: 3 Weeks 

• Stage 1: 1.5 Weeks (50% of project grade) 

• Stage 2: 6 Days (25% of project grade) 

• Stage 3: 1 Week (25% of project grade) 

1.3 Evaluation Criteria 

• Each stage includes evaluation of both encoding and decoding 

• Extra features: 10% additional marks 

• Performance beating latest BZip2: 10% additional marks (max 2 groups) 4  
Data Compression Project BZip2 Implementation 

2 System Architecture 

2.1 Overall Block Diagram 

Encoding Pipeline: 

\================ 

Input File 

| 

v 

Block Division 

| 

v 

RLE-1 Encoding 

| 

v 

BWT 

| 

v 

MTF 

| 

v 

RLE-2 Encoding 

| 

v 

Huffman Encoding 

| 

v 

Compressed Output 

Figure 1: Encoding Pipeline Architecture 

2.2 Configuration Management 

The system uses a configuration file named config.ini with the following structure: 

1 \[ General \] 

2 block\_size \= 500000 \# Bytes (100 KB to 900 KB ) 

3 rle1\_enabled \= true 

4 bwt\_type \= matrix \# matrix or suffix\_array 

5 mtf\_enabled \= true 

6 rle2\_enabled \= true 

7 huffman\_enabled \= true 

8 

9 \[ Performance \] 

10 benchmark\_mode \= false 

11 output\_metrics \= true 

12 

13 \[ Paths \] 

14 input\_directory \= ./ benchmarks / 

5  
Data Compression Project BZip2 Implementation 

15 output\_directory \= ./ results / 

Listing 1: Configuration File Format 

3 Stage 1 Implementation (1.5 Weeks) 

3.1 Block Division and File Handling 

3.1.1 Requirements 

• Support for very large files 

• Configurable block size (100KB \- 900KB) 

• Configuration file integration 

3.1.2 Data Structures 

1 /\* 

2 \* Structure to hold a single block of data 

3 \*/ 

4 typedef struct { 

5 unsigned char \* data ; // Pointer to block data 

6 size\_t size ; // Current size of block 

7 size\_t original\_size ; // Original size before compression 8 } Block ; 

9 

10 /\* 

11 \* Structure to manage multiple blocks 

12 \*/ 

13 typedef struct { 

14 Block \* blocks ; // Array of blocks 

15 int num\_blocks ; // Number of blocks 

16 size\_t block\_size ; // Configurable block size 17 } BlockManager ; 

Listing 2: Block Division Structures (Provided) 

3.1.3 Function Prototypes 

1 /\* 

2 \* Reads input file and divides into blocks 

3 \* @param filename : Input file path 

4 \* @param block\_size : Size of each block in bytes 

5 \* @return : BlockManager structure containing all blocks 6 \*/ 

7 BlockManager \* divide\_into\_blocks ( const char \* filename , size\_t block\_size ) ; 

8 

9 /\* 

10 \* Reassembles blocks back into original file 

11 \* @param manager : BlockManager containing processed blocks 12 \* @param output\_filename : Path for output file 

13 \* @return : 0 on success , \-1 on failure 

6  
Data Compression Project BZip2 Implementation 

14 \*/ 

15 int reassemble\_blocks ( BlockManager \* manager , const char \* output\_filename ) ; 

16 

17 /\* 

18 \* Frees memory allocated for BlockManager 

19 \* @param manager : Pointer to BlockManager to free 

20 \*/ 

21 void free\_block\_manager ( BlockManager \* manager ) ; 

Listing 3: Block Management Functions 

3.2 RLE-1 Implementation 

3.2.1 Description 

Run-Length Encoding replaces sequences of repeated characters with a count and the character. 

Example: 

Input: ABBBCCCCD 

Output: A3B4C1D 

3.2.2 Function Prototypes 

1 /\* 

2 \* Encodes data using Run \- Length Encoding 

3 \* @param input : Input byte array 

4 \* @param len : Length of input array 

5 \* @param output : Output buffer ( must be pre \- allocated ) 6 \* @param out\_len : Pointer to store output length 

7 \*/ 

8 void rle1\_encode ( unsigned char \* input , size\_t len , 

9 unsigned char \* output , size\_t \* out\_len ) ; 10   
11 /\* 

12 \* Decodes RLE \-1 encoded data 

13 \* @param input : Encoded byte array 

14 \* @param len : Length of encoded data 

15 \* @param output : Output buffer for decoded data 

16 \* @param out\_len : Pointer to store decoded length 

17 \*/ 

18 void rle1\_decode ( unsigned char \* input , size\_t len , 

19 unsigned char \* output , size\_t \* out\_len ) ; Listing 4: RLE-1 Functions 

3.3 Burrows-Wheeler Transform (BWT) 

3.3.1 Description 

The matrix-based BWT creates all cyclic rotations of the input string, sorts them lexi cographically, and outputs the last column. 

7  
Data Compression Project BZip2 Implementation 

3.3.2 Data Structures 

1 /\* 

2 \* Structure for rotation in BWT 

3 \*/ 

4 typedef struct { 

5 char \* rotation ; // Rotation string 

6 int index ; // Original index 

7 } Rotation ; 

Listing 5: BWT Structures 

3.3.3 Function Prototypes 

1 /\* 

2 \* Compares two rotations for sorting 

3 \* @param a: First rotation 

4 \* @param b: Second rotation 

5 \* @return : Comparison result ( \-1 , 0 , 1\) 

6 \*/ 

7 int compare\_rotations ( const void \*a , const void \* b ) ; 

8 

9 /\* 

10 \* Forward BWT transform 

11 \* @param input : Input byte array 

12 \* @param len : Length of input 

13 \* @param output : Output buffer for BWT result 

14 \* @param primary\_index : Pointer to store primary index 15 \*/ 

16 void bwt\_encode ( unsigned char \* input , size\_t len , 

17 unsigned char \* output , int \* primary\_index ) ; 18   
19 /\* 

20 \* Inverse BWT transform 

21 \* @param input : BWT encoded data 

22 \* @param len : Length of encoded data 

23 \* @param primary\_index : Primary index from encoding 

24 \* @param output : Output buffer for original data 

25 \*/ 

26 void bwt\_decode ( unsigned char \* input , size\_t len , 

27 int primary\_index , unsigned char \* output ) ; Listing 6: BWT Functions 

3.4 Stage 1 Evaluation 

Both encoding and decoding for Stage 1 components (RLE-1 and BWT) must be fully functional. The evaluation tests: 

• Correctness of RLE-1 encoding/decoding 

• Correctness of BWT forward/inverse transform 

• Block division and reassembly 

• Configuration file parsing 

8  
Data Compression Project BZip2 Implementation 

4 Stage 2 Implementation (6 Days) 

4.1 Move-to-Front (MTF) Transform 

4.1.1 Description 

MTF encodes data by maintaining a list of symbols and outputting the index of each symbol, then moving it to the front. 

4.1.2 Function Prototypes 

1 /\* 

2 \* Forward MTF transform 

3 \* @param input : Input byte array 

4 \* @param len : Length of input 

5 \* @param output : Output buffer for MTF indices 

6 \*/ 

7 void mtf\_encode ( unsigned char \* input , size\_t len , 

8 unsigned char \* output ) ; 

9 

10 /\* 

11 \* Inverse MTF transform 

12 \* @param input : MTF encoded indices 

13 \* @param len : Length of input 

14 \* @param output : Output buffer for decoded data 

15 \*/ 

16 void mtf\_decode ( unsigned char \* input , size\_t len , 

17 unsigned char \* output ) ; 

Listing 7: MTF Functions 

4.2 RLE-2 Implementation 

4.2.1 Description 

RLE-2 specifically targets the output of MTF, which contains many zeros and small numbers. This is a specialized RLE for the MTF output. 

4.2.2 Function Prototypes 

1 /\* 

2 \* Encodes MTF output using specialized RLE 

3 \* @param input : MTF output array 

4 \* @param len : Length of input 

5 \* @param output : Output buffer 

6 \* @param out\_len : Pointer to store output length 

7 \*/ 

8 void rle2\_encode ( unsigned char \* input , size\_t len , 

9 unsigned char \* output , size\_t \* out\_len ) ; 10   
11 /\* 

12 \* Decodes RLE \-2 encoded data 

13 \* @param input : RLE \-2 encoded data 

14 \* @param len : Length of encoded data 

9  
Data Compression Project BZip2 Implementation 

15 \* @param output : Output buffer for MTF data 

16 \* @param out\_len : Pointer to store output length 

17 \*/ 

18 void rle2\_decode ( unsigned char \* input , size\_t len , 

19 unsigned char \* output , size\_t \* out\_len ) ; Listing 8: RLE-2 Functions 

4.3 Stage 2 Evaluation 

The complete pipeline (RLE-1 → BWT → MTF → RLE-2) must work for both encoding and decoding. 

5 Stage 3 Implementation (1 Week) 

5.1 Canonical Huffman Coding 

5.1.1 Description 

Canonical Huffman coding ensures efficient tree representation by storing only code lengths, not the full tree. 

5.1.2 Data Structures 

1 /\* 

2 \* Huffman code representation 

3 \*/ 

4 typedef struct { 

5 unsigned short code ; // Huffman code 

6 unsigned char length ; // Code length in bits 

7 } HuffmanCode ; 

8 

9 /\* 

10 \* Huffman tree node 

11 \*/ 

12 typedef struct Node { 

13 unsigned char symbol ; // Byte value (0 \-255) 

14 int freq ; // Frequency count 

15 struct Node \* left ; // Left child 

16 struct Node \* right ; // Right child 

17 } HuffmanNode ; 

Listing 9: Huffman Data Structures 

5.1.3 Function Prototypes 

1 /\* 

2 \* Builds Huffman tree from frequency counts 

3 \* @param frequencies : Array of 256 frequency counts 

4 \* @param root : Pointer to store root of Huffman tree 

5 \*/ 

6 void build\_huffman\_tree ( int \* frequencies , HuffmanNode \*\* root ) ; 7 

10  
Data Compression Project BZip2 Implementation 

8 /\* 

9 \* Generates canonical Huffman codes from tree 

10 \* @param root : Root of Huffman tree 

11 \* @param codes : Array of 256 to store generated codes 12 \*/ 

13 void generate\_canonical\_codes ( HuffmanNode \* root , 

14 HuffmanCode \* codes ) ; 

15 

16 /\* 

17 \* Encodes data using Huffman coding 

18 \* @param input : Input byte array 

19 \* @param len : Length of input 

20 \* @param output : Output buffer for compressed data 

21 \* @param out\_len : Pointer to store output length 

22 \*/ 

23 void huffman\_encode ( unsigned char \* input , size\_t len , 

24 unsigned char \* output , size\_t \* out\_len ) ; 25   
26 /\* 

27 \* Decodes Huffman encoded data 

28 \* @param input : Huffman encoded data 

29 \* @param len : Length of encoded data 

30 \* @param output : Output buffer for decoded data 

31 \* @param out\_len : Pointer to store output length 

32 \*/ 

33 void huffman\_decode ( unsigned char \* input , size\_t len , 

34 unsigned char \* output , size\_t \* out\_len ) ; 35   
36 /\* 

37 \* Writes Huffman header ( code lengths ) to output 

38 \* @param codes : Array of Huffman codes 

39 \* @param output : Output buffer 

40 \* @param out\_len : Pointer to update with header size 

41 \*/ 

42 void write\_header ( HuffmanCode \* codes , unsigned char \* output , 43 size\_t \* out\_len ) ; 

44 

45 /\* 

46 \* Encodes data using generated codes 

47 \* @param input : Input data 

48 \* @param len : Length of input 

49 \* @param codes : Huffman codes for each symbol 

50 \* @param output : Output buffer 

51 \* @param out\_len : Pointer to update with encoded size 52 \*/ 

53 void encode\_data ( unsigned char \* input , size\_t len , 

54 HuffmanCode \* codes , unsigned char \* output , 55 size\_t \* out\_len ) ; 

Listing 10: Huffman Functions 

11  
Data Compression Project BZip2 Implementation 

6 Build System and Platform Support 

6.1 Makefile Requirements 

The project must include a cross-platform Makefile supporting both Linux and Windows. The Makefile should have the following targets: 

1 \# Required Makefile targets : 

2 \# all \- Compile the complete project 

3 \# clean \- Remove object files and executable 

4 \# windows \- Cross \- compile for Windows platform 

5 

6 \# Example structure ( students must complete ) : 

7 CC \= gcc 

8 CFLAGS \= \- Wall \- O2 

9 TARGET \= bzip2\_impl 

10 SOURCES \= main . c rle . c bwt . c mtf . c huffman . c config . c 

11 

12 \# Platform detection and flags must be implemented 

13 \# Windows cross \- compilation must be supported 

Listing 11: Makefile Structure (Students must implement) 

7 Performance Evaluation and Benchmarking 

7.1 Benchmark Datasets 

Students must use the following publicly available benchmark files for testing: Table 1: Benchmark Datasets 

| Dataset  | Size  | Type |
| ----- | ----- | ----- |
| Canterbury Corpus  | Various  | Mixed |
| Calgary Corpus  | Various  | Mixed |
| Silesia Corpus  | Various  | Mixed |
| Large Text Files  | 10MB-100MB  | Text |
| Binary Files  | 10MB-100MB  | Binary |

7.2 Performance Metrics 

Performance is evaluated using a weighted formula: 

Score \= *w*1 *·C*ref   
*C*\+ *w*2 *·SS*ref   
Where: 

• *C* \= Compression ratio achieved 

• *C*ref \= Reference compression ratio (bzip2) 

• *S* \= Speed (MB/s) 

12  
Data Compression Project BZip2 Implementation 

• *S*ref \= Reference speed (bzip2) 

• *w*1*, w*2 \= Weights (to be determined by instructor) 

7.3 Automated Results Generation 

Students must implement a script to automatically generate results in results.csv with the following format: 

1 \# results . csv must contain : 

2 File , Size , BlockSize , CompressionRatio , Time , Memory 

3 

4 \# Students must implement their own benchmarking script Listing 12: Required Results Format 

8 Extra Features and Enhancements (10% Marks) These will be 10% marks of the overall project grade. 

8.1 Enhanced RLE Versions (Extra Marks) 

Students can implement: 

• RLE with run-length thresholds: Only encode runs longer than a configurable threshold 

• Adaptive RLE: Dynamically adjust encoding based on data characteristics 

• RLE with entropy coding: Combine with simple entropy coding for better compression 

8.2 Suffix Array-based BWT (Extra Marks) 

Faster implementation using suffix array algorithm: 

1 /\* 

2 \* Builds suffix array for efficient BWT 

3 \* @param text : Input text 

4 \* @param n: Length of text 

5 \* @return : Suffix array 

6 \*/ 

7 int \* build\_suffix\_array ( unsigned char \* text , int n ) ; 

Listing 13: Suffix Array BWT Prototype 

8.3 Alternative Entropy Coding 

• ANS (Asymmetric Numeral Systems): Modern entropy coder with near optimal compression 

• Range Coding: Arithmetic coding alternative with integer precision 13  
Data Compression Project BZip2 Implementation 

9 Submission Guidelines 

9.1 GitHub Repository Structure 

project-bzip2/ 

|-- src/ 

| |-- main.c 

| |-- rle.c 

| |-- bwt.c 

| |-- mtf.c 

| |-- huffman.c 

| ‘-- config.c 

|-- include/ 

| ‘-- \*.h 

|-- benchmarks/ 

| ‘-- (test files) 

|-- results/ 

| ‘-- (output files) 

|-- Makefile 

|-- config.ini 

|-- README.md 

‘-- report.pdf 

9.2 README.md Requirements 

The README must include: 

• Complete feature description 

• Implementation details for each stage (written by students) 

• Build instructions for Linux and Windows 

• Usage examples 

• Performance results and graphs 

• Team member names and contributions 

9.3 Graph Generation 

Students must implement a script to generate performance graphs. Example structure: 

1 \#\!/ usr / bin / env python3 

2 \# Students must implement their own graph generation 

3 \# Should read results . csv and produce graphs 

4 

5 import matplotlib . pyplot as plt 

6 import pandas as pd 

7 

8 \# Students implement their own plotting logic 

Listing 14: Plot Generation Script Structure 

14  
Data Compression Project BZip2 Implementation 

10 Important Notes for Students 

• No code implementation is provided \- you must write all functions yourself • Only struct definitions and function prototypes are given as guidance • You may discuss algorithms but must write your own code 

• Plagiarism will result in grade penalty 

• Each team member must contribute to the implementation 

• Submit GitHub repository link with all required files 

11 Conclusion 

This project requires implementing a complete BZip2-like compression pipeline with con figurable parameters, cross-platform support, and comprehensive benchmarking capabil ities. Students must write all code themselves based on the provided specifications and prototypes. 

References 

\[1\] Seward, J. (1996). The bzip2 and libbzip2 official home page. 

\[2\] Burrows, M., & Wheeler, D. J. (1994). A block-sorting lossless data compression algorithm. 

\[3\] Huffman, D. A. (1952). A method for the construction of minimum-redundancy codes. 

\[4\] Manber, U., & Myers, G. (1993). Suffix arrays: a new method for on-line string searches. 

15