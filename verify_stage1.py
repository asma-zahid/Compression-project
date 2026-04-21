"""
Stage 1 pipeline verifier - mirrors exact C logic in bwt.c and rle.c
Run with:  python verify_stage1.py
"""

# ── RLE-1 (mirrors rle.c) ─────────────────────────────────────────
def rle1_encode(data):
    out = []; i = 0
    while i < len(data):
        curr = data[i]; count = 1
        while i + count < len(data) and data[i + count] == curr and count < 255:
            count += 1
        out.append(count)   # count byte first
        out.append(curr)    # then value byte
        i += count
    return bytes(out)

def rle1_decode(data):
    out = []; i = 0
    while i + 1 < len(data):
        count = data[i]; byte = data[i + 1]; i += 2
        out.extend([byte] * count)
    return bytes(out)

# ── BWT (mirrors bwt.c) ───────────────────────────────────────────
def bwt_encode(data):
    n = len(data)
    # sort rotation indices lexicographically
    indices = sorted(range(n),
                     key=lambda j: bytes(data[(j + k) % n] for k in range(n)))
    # last column = char before each sorted rotation's start
    last_col    = bytes(data[(indices[i] + n - 1) % n] for i in range(n))
    primary_idx = next(i for i, idx in enumerate(indices) if idx == 0)
    return last_col, primary_idx

def bwt_decode(L, primary):
    n = len(L)
    # frequency counts
    freq = [0] * 256
    for b in L:
        freq[b] += 1
    # cumulative starts (C array)
    C = [0] * 256
    total = 0
    for c in range(256):
        C[c] = total; total += freq[c]
    # LF mapping
    rank = [0] * 256
    LF   = [0] * n
    for i, c in enumerate(L):
        LF[i] = C[c] + rank[c]; rank[c] += 1
    # reconstruct
    out = [0] * n; idx = primary
    for i in range(n - 1, -1, -1):
        out[i] = L[idx]; idx = LF[idx]
    return bytes(out)

# ── Test cases ────────────────────────────────────────────────────
tests = [
    (b"BANANA",                    "classic BWT example"),
    (b"ABBBCCCCD",                 "spec RLE-1 example"),
    (b"AAABBBCCC",                 "grouped runs"),
    (b"Hello, World!",             "mixed text"),
    (b"AAAAAAAAAA",                "all same bytes"),
    (b"ABCDEFGHIJ",                "all unique bytes"),
    (b"\x00\x01\x02\xfe\xff",      "binary bytes incl 0x00"),
    (b"A" * 260,                   "run > 255 (must split to 255+5)"),
    (b"A" * 255 + b"B" * 255,      "two maximal runs"),
    (b"The quick brown fox",        "normal sentence"),
]

print("=" * 60)
print("Stage 1 Pipeline Verification")
print("=" * 60)

all_pass = True
for original, label in tests:
    # RLE-1 round-trip
    enc = rle1_encode(original)
    rle_ok = rle1_decode(enc) == original

    # BWT round-trip
    bwt_out, pidx = bwt_encode(original)
    bwt_ok = bwt_decode(bwt_out, pidx) == original

    # Full pipeline: RLE-1 -> BWT -> iBWT -> iRLE-1
    r1  = rle1_encode(original)
    bw, p2 = bwt_encode(r1)
    pipe_ok = rle1_decode(bwt_decode(bw, p2)) == original

    ok = rle_ok and bwt_ok and pipe_ok
    if not ok:
        all_pass = False
    tag = "PASS" if ok else "FAIL"
    print(f"[{tag}] {label}")
    if not ok:
        print(f"       rle_ok={rle_ok}  bwt_ok={bwt_ok}  pipe_ok={pipe_ok}")

print("=" * 60)
print("ALL TESTS PASSED" if all_pass else "SOME TESTS FAILED")
print("=" * 60)

# Show the BANANA BWT step-by-step for documentation
print()
print("BWT detail check (BANANA):")
L, p = bwt_encode(b"BANANA")
print(f"  Input        : BANANA")
print(f"  BWT output   : {L.decode()}")
print(f"  Primary index: {p}")
recovered = bwt_decode(L, p)
print(f"  Recovered    : {recovered.decode()}")
