#!/usr/bin/env bash
# tools/run_benchmark.sh
#
# Compress every file in benchmarks/ at several block sizes, capturing
# wall-clock time, peak resident memory, and compression ratio.  Writes
# results.csv in the format required by project.md §7.3:
#
#     File,Size,BlockSize,CompressionRatio,Time,Memory
#
# Memory is in KB (max RSS, from /usr/bin/time -f '%M').
# Time     is wall-clock seconds.
# Ratio    is original_size / compressed_size  (higher is better).
#
# Run from the project root:   bash tools/run_benchmark.sh

set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

BIN="$ROOT/bzip2_impl"
[[ -x "$BIN" ]] || { echo "Binary not built — run 'make' first" >&2; exit 1; }
command -v /usr/bin/time >/dev/null || { echo "/usr/bin/time required" >&2; exit 1; }

OUT="$ROOT/results/results.csv"
mkdir -p "$ROOT/results"
echo "File,Size,BlockSize,CompressionRatio,Time,Memory" > "$OUT"

# Force suffix-array BWT for the run (matrix BWT is impractical >10 KB).
sed -i 's/^bwt_type.*/bwt_type = suffix_array/' config.ini

# Block sizes covering the spec range 100 KB - 900 KB.
BLOCK_SIZES=(102400 250000 500000 750000 921600)

# Files to test — sort by size so quick ones print first.
FILES=$(ls -S benchmarks/* 2>/dev/null | tac)

TMP_TIME=$(mktemp)
TMP_OUT=$(mktemp)
trap 'rm -f "$TMP_TIME" "$TMP_OUT"' EXIT

printf '%-18s %-8s %8s %8s %10s %10s\n' 'File' 'BlkSize' 'Size' 'Comp.' 'Ratio' 'Time(s)'
echo  '------------------------------------------------------------------------'

for file in $FILES; do
    [[ -f "$file" ]] || continue
    base=$(basename "$file")
    size=$(stat -c %s "$file")

    for bs in "${BLOCK_SIZES[@]}"; do
        sed -i "s/^block_size.*/block_size = $bs/" config.ini

        # /usr/bin/time -f '%e %M'  →  wall-seconds  max-RSS-KB
        /usr/bin/time -f '%e %M' -o "$TMP_TIME" \
            "$BIN" compress "$file" "$TMP_OUT" >/dev/null 2>&1 || {
                echo "  [error compressing $base @ bs=$bs]" >&2
                continue
            }

        elapsed=$(awk '{print $1}' "$TMP_TIME")
        max_rss=$(awk '{print $2}' "$TMP_TIME")
        comp=$(stat -c %s "$TMP_OUT")
        ratio=$(awk "BEGIN { printf \"%.4f\", $size / $comp }")

        printf '%-18s %-8d %8d %8d %10s %10s\n' \
            "$base" "$bs" "$size" "$comp" "$ratio" "$elapsed"
        echo "$base,$size,$bs,$ratio,$elapsed,$max_rss" >> "$OUT"
    done
done

echo
echo "Wrote $OUT"
echo "Rows: $(($(wc -l < "$OUT") - 1))"
