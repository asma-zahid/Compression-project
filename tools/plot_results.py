#!/usr/bin/env python3
"""Plot benchmark results from results/results.csv.

Produces results/benchmark.png with four panels:
  1. Compression ratio per file (averaged over block sizes).
  2. Compression time vs input size (log-log).
  3. Compression ratio vs block size (per file).
  4. Peak memory vs input size.

Run from the project root:   python3 tools/plot_results.py
"""
from __future__ import annotations

import os
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


ROOT = Path(__file__).resolve().parent.parent
CSV  = ROOT / "results" / "results.csv"
PNG  = ROOT / "results" / "benchmark.png"


def main() -> int:
    if not CSV.exists():
        sys.stderr.write(f"missing {CSV} — run tools/run_benchmark.sh first\n")
        return 1

    df = pd.read_csv(CSV)
    df["Memory_MB"] = df["Memory"] / 1024.0

    fig, ax = plt.subplots(2, 2, figsize=(12, 8))
    fig.suptitle("BZip2 Implementation – Canterbury Corpus Benchmark", fontsize=13)

    # 1. Average ratio per file
    ratio_avg = df.groupby("File")["CompressionRatio"].mean().sort_values()
    ratio_avg.plot.barh(ax=ax[0, 0], color="#4c72b0")
    ax[0, 0].set_title("Compression ratio (avg over block sizes)")
    ax[0, 0].set_xlabel("Ratio (higher = better)")
    ax[0, 0].axvline(1.0, color="gray", linestyle="--", linewidth=0.8)

    # 2. Time vs input size (log-log)
    for f, sub in df.groupby("File"):
        ax[0, 1].plot(sub["Size"], sub["Time"], marker="o", label=f, alpha=0.7)
    ax[0, 1].set_xscale("log")
    ax[0, 1].set_yscale("log")
    ax[0, 1].set_xlabel("Input size (bytes)")
    ax[0, 1].set_ylabel("Compression time (s)")
    ax[0, 1].set_title("Time vs input size")
    ax[0, 1].grid(True, which="both", alpha=0.3)
    ax[0, 1].legend(fontsize=7, ncol=2)

    # 3. Ratio vs block size per file
    for f, sub in df.groupby("File"):
        sub = sub.sort_values("BlockSize")
        ax[1, 0].plot(sub["BlockSize"], sub["CompressionRatio"],
                      marker="o", label=f, alpha=0.7)
    ax[1, 0].set_xlabel("Block size (bytes)")
    ax[1, 0].set_ylabel("Compression ratio")
    ax[1, 0].set_title("Ratio vs block size")
    ax[1, 0].grid(True, alpha=0.3)
    ax[1, 0].legend(fontsize=7, ncol=2)

    # 4. Memory vs input size
    for bs, sub in df.groupby("BlockSize"):
        ax[1, 1].scatter(sub["Size"], sub["Memory_MB"],
                         label=f"bs={bs}", alpha=0.7, s=30)
    ax[1, 1].set_xlabel("Input size (bytes)")
    ax[1, 1].set_ylabel("Peak memory (MB)")
    ax[1, 1].set_title("Memory vs input size")
    ax[1, 1].set_xscale("log")
    ax[1, 1].grid(True, alpha=0.3)
    ax[1, 1].legend(fontsize=7)

    plt.tight_layout(rect=(0, 0, 1, 0.96))
    PNG.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(PNG, dpi=120)
    print(f"wrote {PNG.relative_to(ROOT)}  ({os.path.getsize(PNG)//1024} KB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
