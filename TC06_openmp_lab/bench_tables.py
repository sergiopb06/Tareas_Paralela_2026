#!/usr/bin/env python3
"""
bench_tables.py - runs pipeline_omp several times per thread count and prints
the two measurement tables of the README, using the MEDIAN of the runs.

This script was written with the help of Claude (Sonnet 5.5):
Anthropic. (2026). Claude (Sonnet 5.5) [Large language model]. https://claude.ai

Usage:
  python3 bench_tables.py                          # data/input.ppm, 1,2,4,8, 11 runs
  python3 bench_tables.py data/input.ppm 1,2,4,8   # choose threads
  python3 bench_tables.py data/small.ppm 1,4 21    # image, threads, runs
The first thread count in the list is used as the speedup baseline.
"""
import re
import statistics as st
import subprocess
import sys

IMG  = sys.argv[1] if len(sys.argv) > 1 else "data/input.ppm"
THR  = [int(x) for x in (sys.argv[2] if len(sys.argv) > 2 else "1,2,4,8").split(",")]
RUNS = int(sys.argv[3]) if len(sys.argv) > 3 else 11

PATTERNS = {
    "load":      r"load\s+([\d.]+) s",
    "gray":      r"stage 1 gray\s+([\d.]+)",
    "blur":      r"stage 2 blur\s+([\d.]+)",
    "sobel":     r"stage 3 sobel\s+([\d.]+)",
    "hist":      r"stage 4 hist\s+([\d.]+)",
    "save":      r"save\s+([\d.]+) s",
    "total":     r"total\s+([\d.]+) s",
    "naive":     r"naive\s+([\d.]+)",
    "critical":  r"critical\s+([\d.]+)",
    "atomic":    r"atomic\s+([\d.]+)",
    "reduction": r"reduction\s+([\d.]+) s\s+wrong",
    "P":         r"P = ([\d.]+)",
    "lost":      r"counts lost (-?\d+)",
}

def run_once(threads):
    out = subprocess.run(["./bin/pipeline_omp", IMG, str(threads)],
                         capture_output=True, text=True, check=True).stdout
    return {k: float(re.search(p, out).group(1)) for k, p in PATTERNS.items()}

lines = []
def emit(s=""):
    print(s)
    lines.append(s)

data = {}
for t in THR:
    print(f"running {RUNS} times with {t} thread(s)...", file=sys.stderr)
    data[t] = [run_once(t) for _ in range(RUNS)]

med  = {t: {k: st.median(r[k] for r in rs) for k in PATTERNS} for t, rs in data.items()}
base = med[THR[0]]["total"]

emit(f"Image: {IMG} | {RUNS} runs per configuration\n")
emit("Table 1\n")
emit("| threads | gray | blur | sobel | hist (reduction) | total | speedup |")
emit("|---|---|---|---|---|---|---|")
for t in THR:
    m = med[t]
    emit(f"| {t} | {m['gray']:.3f} | {m['blur']:.3f} | {m['sobel']:.3f} | "
         f"{m['hist']:.3f} | {m['total']:.3f} | {base / m['total']:.2f}x |")

emit("\nTable 2 (stage 4 only)\n")
emit("| threads | naive | critical | atomic | reduction |")
emit("|---|---|---|---|---|")
for t in THR:
    m = med[t]
    emit(f"| {t} | {m['naive']:.3f} | {m['critical']:.3f} | {m['atomic']:.3f} | {m['reduction']:.3f} |")

emit("\nSerial part (load, save) per thread count\n")
emit("| threads | load | save | load+save | total | (load+save)/total |")
emit("|---|---|---|---|---|---|")
for t in THR:
    m = med[t]
    s = m["load"] + m["save"]
    emit(f"| {t} | {m['load']:.3f} | {m['save']:.3f} | {s:.3f} | {m['total']:.3f} | {s / m['total']:.3f} |")

p = med[THR[0]]["P"]
emit(f"\nP measured with {THR[0]} thread(s) (median): {p:.3f}  ->  Amdahl ceiling 1/(1-P) = {1 / (1 - p):.2f}x")
emit("S(n) = 1 / ((1-P) + P/n):")
for n in (2, 4, 6, 8, 12, 16):
    emit(f"  n={n:<2}  {1 / ((1 - p) + p / n):.2f}x")

emit("\nCounts lost by the naive version in every run:")
for t in THR:
    emit(f"  {t} thread(s): {[int(r['lost']) for r in data[t]]}")

with open("results.md", "w") as f:
    f.write("\n".join(lines) + "\n")
print("\nSaved to results.md", file=sys.stderr)
