#!/usr/bin/env python3
"""Stdlib-only fallback plotter (no pandas / matplotlib needed).

Reads results/results.csv and emits results/benchmark.svg with two
panels:
  - bar chart of average compression ratio per file
  - bar chart of average compression time per file

Run:   python3 tools/plot_results_svg.py
"""
from __future__ import annotations

import csv
import os
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CSV  = ROOT / "results" / "results.csv"
SVG  = ROOT / "results" / "benchmark.svg"


def load_rows(path: Path) -> list[dict]:
    with path.open() as f:
        return [
            {
                "File":  row["File"],
                "Size":  int(row["Size"]),
                "BlockSize": int(row["BlockSize"]),
                "Ratio": float(row["CompressionRatio"]),
                "Time":  float(row["Time"]),
                "Mem":   int(row["Memory"]),
            }
            for row in csv.DictReader(f)
        ]


def avg_by(rows: list[dict], key: str, value: str) -> dict[str, float]:
    bucket: dict[str, list[float]] = defaultdict(list)
    for r in rows:
        bucket[r[key]].append(r[value])
    return {k: sum(v) / len(v) for k, v in bucket.items()}


def bar_panel(
    title: str,
    data: dict[str, float],
    *,
    x_offset: int,
    y_offset: int,
    width: int,
    height: int,
    color: str,
    fmt: str = "{:.2f}",
) -> str:
    items = sorted(data.items(), key=lambda kv: kv[1])
    n = len(items)
    if n == 0:
        return ""

    pad_left   = 130
    pad_right  = 80
    pad_top    = 30
    pad_bottom = 20
    chart_w = width  - pad_left - pad_right
    chart_h = height - pad_top  - pad_bottom

    max_val = max(v for _, v in items)
    bar_h   = chart_h / n * 0.75
    gap_h   = chart_h / n * 0.25

    parts = [
        f'<g transform="translate({x_offset},{y_offset})">',
        f'<text x="{width//2}" y="20" text-anchor="middle" '
        f'font-family="sans-serif" font-size="13" font-weight="bold">{title}</text>',
    ]

    for i, (name, val) in enumerate(items):
        y  = pad_top + i * (bar_h + gap_h)
        bw = (val / max_val) * chart_w if max_val > 0 else 0
        parts.append(
            f'<rect x="{pad_left}" y="{y:.1f}" width="{bw:.1f}" height="{bar_h:.1f}" '
            f'fill="{color}" />'
        )
        parts.append(
            f'<text x="{pad_left - 5}" y="{y + bar_h * 0.7:.1f}" text-anchor="end" '
            f'font-family="monospace" font-size="10">{name}</text>'
        )
        parts.append(
            f'<text x="{pad_left + bw + 4:.1f}" y="{y + bar_h * 0.7:.1f}" '
            f'font-family="monospace" font-size="10" fill="#333">{fmt.format(val)}</text>'
        )

    parts.append("</g>")
    return "\n".join(parts)


def main() -> int:
    if not CSV.exists():
        sys.stderr.write(f"missing {CSV} — run tools/run_benchmark.sh first\n")
        return 1

    rows = load_rows(CSV)
    if not rows:
        sys.stderr.write("results.csv is empty\n")
        return 1

    avg_ratio = avg_by(rows, "File", "Ratio")
    avg_time  = avg_by(rows, "File", "Time")

    width   = 600
    height  = 360
    total_w = width * 2 + 20
    total_h = height + 40

    panels = [
        bar_panel(
            "Compression ratio (avg over block sizes)",
            avg_ratio,
            x_offset=0, y_offset=20,
            width=width, height=height,
            color="#4c72b0",
            fmt="{:.2f}x",
        ),
        bar_panel(
            "Compression time (s, avg over block sizes)",
            avg_time,
            x_offset=width + 20, y_offset=20,
            width=width, height=height,
            color="#dd8452",
            fmt="{:.2f}s",
        ),
    ]

    svg = f"""<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{total_w}" height="{total_h}"
     viewBox="0 0 {total_w} {total_h}">
  <rect width="100%" height="100%" fill="white"/>
  <text x="{total_w//2}" y="14" text-anchor="middle"
        font-family="sans-serif" font-size="13" font-weight="bold"
        fill="#222">BZip2 Implementation – Canterbury Corpus Benchmark</text>
{chr(10).join(panels)}
</svg>
"""

    SVG.parent.mkdir(parents=True, exist_ok=True)
    SVG.write_text(svg)
    print(f"wrote {SVG.relative_to(ROOT)}  ({os.path.getsize(SVG)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
