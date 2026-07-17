#!/usr/bin/env python3
"""
Plot histogram_*.out / coarse_histogram_*.out files produced by timing_histogram.c,
reproducing the paper's per-machine figure style: one subplot each for read/write/flush
access, with all 22 timing categories overlaid, each normalized to its own peak (0-100%),
and legend entries of the form "{n}. ShortLabel (peak_cycle)".

Each input file is a tab-separated matrix: one row per cycle bin, one column per
timing "calibre" type (NUM_CALIBRE = 66 columns = 22 categories x {read, write, flush}).
histogram_*.out has one row per cycle (0..MAX_CYCLE-1); coarse_histogram_*.out bins
every 10 cycles (0..MAX_CYCLE/10-1). Column blocks, per timing_histogram.c's main():
  columns  0-21: READ
  columns 22-43: WRITE
  columns 44-65: FLUSH (CLFLUSH)
and within each block of 22, column order matches the #defines in timing_histogram.c
(L1_HIT_CLEAN, L2_HIT_CLEAN, ..., L3_REMOTE_L3_HIT_CLEAN).

Usage:
    python3 plot_histogram.py
    python3 plot_histogram.py histogram_output/histogram_my_machine.out
    python3 plot_histogram.py histogram_output/*.out --xmax 2000
"""

import argparse
import glob
import os

import numpy as np
import matplotlib.pyplot as plt

# Short labels for the 22 timing categories, in column order, matching the paper figure.
SHORT_LABELS = [
    "clean L1", "clean L2", "clean L3", "clean remote L1", "clean remote L2", "clean remote L3",
    "dirty L1", "dirty L2", "dirty L3", "dirty remote L1", "dirty remote L2", "dirty remote L3",
    "DRAM",
    "clean L1 & remote L1", "clean L1 & remote L2", "clean L1 & remote L3",
    "clean L2 & remote L1", "clean L2 & remote L2", "clean L2 & remote L3",
    "clean L3 & remote L1", "clean L3 & remote L2", "clean L3 & remote L3",
]

BLOCK_TITLES = ["read access", "write access", "flush operation"]

# 22 visually distinct colors, reused across all three subplots so the same
# category (e.g. L1cl) always gets the same color.
COLORS = plt.get_cmap("tab20").colors + plt.get_cmap("Set1").colors[:2]


def load_matrix(path):
    matrix = np.loadtxt(path)
    if matrix.ndim == 1:
        matrix = matrix.reshape(-1, 1)
    return matrix


def is_coarse(path):
    return "coarse" in os.path.basename(path)


def machine_name(path):
    base = os.path.basename(path)
    base = base[:-4] if base.endswith(".out") else base
    for prefix in ("coarse_histogram_", "histogram_"):
        if base.startswith(prefix):
            return base[len(prefix):]
    return base


def plot_block(ax, x, bin_width, block, title, xmax, index_offset):
    for k in range(block.shape[1]):
        col = block[:, k]
        peak_idx = int(np.argmax(col))
        label = "{%d} %s" % (index_offset + k + 1, SHORT_LABELS[k])
        ax.bar(x, col, width=bin_width, color=COLORS[k % len(COLORS)], label=label, linewidth=0)

    ax.set_xlim(0, xmax)
    ax.set_xticks(range(0, xmax + 1, max(100, xmax // 20)))
    ax.set_ylabel("Cycles")
    ax.set_title(title)
    ax.legend(fontsize=6, ncol=4, loc="upper right")


def plot_file(path, outdir, show, xmax):
    matrix = load_matrix(path)
    num_rows, num_cols = matrix.shape
    if num_cols != 66:
        print("Skipping %s: expected 66 columns (22 categories x read/write/flush), got %d" % (path, num_cols))
        return

    bin_width = 10 if is_coarse(path) else 1
    x = np.arange(num_rows) * bin_width
    xmax = min(xmax, int(x[-1]))

    machine = machine_name(path)
    fig, axes = plt.subplots(3, 1, figsize=(12, 13))
    for block_idx in range(3):
        block = matrix[:, block_idx * 22:(block_idx + 1) * 22]
        title = "Timing of %s on %s" % (BLOCK_TITLES[block_idx], machine)
        plot_block(axes[block_idx], x, bin_width, block, title, xmax, index_offset=block_idx * 22)
    fig.tight_layout()

    os.makedirs(outdir, exist_ok=True)
    out_path = os.path.join(outdir, os.path.basename(path).replace(".out", ".png"))
    fig.savefig(out_path, dpi=150)
    print("Wrote %s" % out_path)

    if show:
        plt.show()
    plt.close(fig)


def default_files():
    files = sorted(glob.glob("histogram_output/histogram_*.out"))
    files += sorted(glob.glob("histogram_output/coarse_histogram_*.out"))
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", nargs="*", help="histogram_*.out / coarse_histogram_*.out files (default: everything under histogram_output/)")
    parser.add_argument("--xmax", type=int, default=2000, help="max cycle count shown on the x-axis (default: 2000, matching the paper figure)")
    parser.add_argument("--outdir", default="gen_figure", help="directory to save PNGs into (default: gen_figure)")
    parser.add_argument("--no-show", action="store_true", help="only save PNGs, don't open interactive windows")
    args = parser.parse_args()

    files = args.files or default_files()
    if not files:
        parser.error("no files given and none found under histogram_output/")

    for path in files:
        plot_file(path, args.outdir, show=not args.no_show, xmax=args.xmax)


if __name__ == "__main__":
    main()
