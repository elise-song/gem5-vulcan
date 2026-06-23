import os
import re
import sys
import glob
import csv

# usage: python3 all_reports.py [data_dir] [output_prefix]
#   data_dir: directory containing *.report.txt files (default: configs/vulcan/data)
#   output_prefix: prefix for output files (default: configs/vulcan/data/summary)
#
# Expects report filenames of the form:
#   <size>.assoc<N>.<i>.report.txt
# e.g. 16KiB.assoc2.3.report.txt
#
# Each report file is expected to contain a line:
#   success_rate = <num>/<den> = <float>

FILENAME_RE = re.compile(r"^(.+)\.assoc(\d+)\.(\d+)\.report\.txt$")
RATE_RE = re.compile(r"success_rate\s*=\s*\d+/\d+\s*=\s*([0-9.]+)")

SIZE_ORDER = {
    "128B": 128,
    "256B": 256,
    "512B": 512,
    "1KiB": 1024,
    "2KiB": 2048,
    "4KiB": 4096,
    "8KiB": 8192,
    "16KiB": 16384,
    "32KiB": 32768,
    "64KiB": 65536,
    "128KiB": 131072,
    "256KiB": 262144,
    "512KiB": 524288,
    "1MiB": 1048576,
}


def parse_rate_from_file(path):
    with open(path, "r") as f:
        for line in f:
            m = RATE_RE.search(line)
            if m:
                return float(m.group(1))
    return None


def main():
    data_dir = sys.argv[1] if len(sys.argv) > 1 else "configs/vulcan/data"
    out_prefix = sys.argv[2] if len(sys.argv) > 2 else os.path.join(data_dir, "summary")

    pattern = os.path.join(data_dir, "*.report.txt")
    files = glob.glob(pattern)

    if not files:
        print(f"No report files found matching {pattern}")
        return

    # results[(size, assoc)] = list of rates
    results = {}

    for path in files:
        fname = os.path.basename(path)
        m = FILENAME_RE.match(fname)
        if not m:
            print(f"Skipping unrecognized filename: {fname}")
            continue

        size, assoc_str, idx_str = m.groups()
        assoc = int(assoc_str)

        rate = parse_rate_from_file(path)
        if rate is None:
            print(f"Could not parse success_rate from {path}")
            continue

        results.setdefault((size, assoc), []).append(rate)

    if not results:
        print("No valid results parsed.")
        return

    rows = []
    for (size, assoc), rates in results.items():
        avg = sum(rates) / len(rates)
        rows.append((size, assoc, len(rates), avg))

    def sort_key(row):
        size, assoc, _, _ = row
        size_bytes = SIZE_ORDER.get(size, float("inf"))
        return (size_bytes, assoc)

    rows.sort(key=sort_key)

    csv_path = out_prefix + ".csv"
    with open(csv_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["cache_size", "assoc", "num_runs", "avg_success_rate"])
        for size, assoc, n, avg in rows:
            writer.writerow([size, assoc, n, f"{avg:.4f}"])

    print(f"Wrote summary CSV to {csv_path}")

    txt_path = out_prefix + ".txt"
    with open(txt_path, "w") as f:
        f.write(f"{'cache_size':<10} {'assoc':<6} {'num_runs':<9} {'avg_success_rate':<18}\n")
        f.write("-" * 45 + "\n")
        for size, assoc, n, avg in rows:
            f.write(f"{size:<10} {assoc:<6} {n:<9} {avg:<18.4f}\n")

    print(f"Wrote summary table to {txt_path}")

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not available; skipping plot. "
              "Install with: pip install matplotlib --break-system-packages")
        return

    by_assoc = {}
    for size, assoc, n, avg in rows:
        by_assoc.setdefault(assoc, []).append((SIZE_ORDER.get(size, 0), size, avg))

    fig, ax = plt.subplots(figsize=(8, 5))

    for assoc in sorted(by_assoc.keys()):
        points = sorted(by_assoc[assoc], key=lambda p: p[0])
        xs = [p[0] for p in points]
        ys = [1 - p[2] for p in points]  # defense success = 1 - attacker success
        labels = [p[1] for p in points]
        ax.plot(xs, ys, marker="o", label=f"assoc={assoc}")

    ax.set_xscale("log", base=2)
    ax.set_xlabel("Cache size (bytes)")
    ax.set_ylabel("Defense success rate")
    ax.set_title("Cache Line Locking Defense Effectiveness")
    ax.set_ylim(-0.05, 1.05)
    ax.legend(title="Associativity")
    ax.grid(True, which="both", linestyle="--", alpha=0.5)

    all_sizes = sorted(set((SIZE_ORDER.get(size, 0), size) for size, _, _, _ in rows))
    ax.set_xticks([s[0] for s in all_sizes])
    ax.set_xticklabels([s[1] for s in all_sizes], rotation=45, ha="right")

    fig.tight_layout()

    plot_path = out_prefix + ".png"
    fig.savefig(plot_path, dpi=150)
    print(f"Wrote plot to {plot_path}")


if __name__ == "__main__":
    main()