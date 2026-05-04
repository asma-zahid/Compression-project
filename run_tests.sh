#!/bin/bash
# Stage 3 roundtrip suite — run from project root after `make`
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
BIN="$HERE/bzip2_impl"
IN=/tmp/_xc.in
BZ=/tmp/_xc.bz
OUT=/tmp/_xc.out

run_test() {
    local name="$1"
    local in_size; in_size=$(stat -c %s "$IN")
    "$BIN" compress   "$IN" "$BZ"  > /dev/null
    "$BIN" decompress "$BZ" "$OUT" > /dev/null
    local bz_size; bz_size=$(stat -c %s "$BZ")
    if cmp -s "$IN" "$OUT"; then
        printf '[PASS] %-32s  in=%-7d  bz=%-7d  ratio=%.2fx\n' \
            "$name" "$in_size" "$bz_size" \
            "$(python3 -c "print($in_size/$bz_size)")"
    else
        printf '[FAIL] %s  roundtrip differs\n' "$name"
    fi
}

printf 'B' > "$IN"; run_test "single byte"
printf '\x00' > "$IN"; run_test "single zero byte"
python3 -c "import sys; sys.stdout.buffer.write(b'\\x00'*1000)" > "$IN"; run_test "all zeros (1000)"
python3 -c "import sys; sys.stdout.buffer.write(b'\\xff'*1000)" > "$IN"; run_test "all 0xFF (1000)"
python3 -c "import sys; sys.stdout.buffer.write(bytes(range(256))*20)" > "$IN"; run_test "incrementing 0..255 x20"
python3 -c "import sys; sys.stdout.buffer.write(bytes([0,1]*500))" > "$IN"; run_test "alternating 0,1 (1000)"
python3 -c "import sys; sys.stdout.buffer.write(b'\\x00'*1024)" > "$IN"; run_test "long zero run (1024)"
python3 -c "import sys; sys.stdout.buffer.write(b'the quick brown fox jumps over the lazy dog. '*100)" > "$IN"; run_test "repeated English text"
python3 -c "import sys, random; random.seed(42); sys.stdout.buffer.write(bytes(random.randint(0,255) for _ in range(1000)))" > "$IN"; run_test "pseudorandom 1000B"
printf 'banana' > "$IN"; run_test "banana"

rm -f "$IN" "$BZ" "$OUT"
