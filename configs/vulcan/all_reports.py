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
#   <size>.assoc<N>.<lock_mode>.<i>.report.txt
# e.g. 16KiB.assoc2.locked.3.report.txt
#
# Each report file is expected to contain a line:
#   success_rate = <num>/<den> = <float>

FILENAME_RE = re.compile(r"^(.+)\.assoc(\d+)\.(locked|nolock|unlocked)\.(\d+)\.report\.txt$")
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

    # results[(size, assoc, lock_mode)] = list of rates
    results = {}

    for path in files:
        fname = os.path.basename(path)
        m = FILENAME_RE.match(fname)
        if not m:
            print(f"Skipping unrecognized filename: {fname}")
            continue

        size, assoc_str, lock_mode, idx_str = m.groups()
        assoc = int(assoc_str)

        rate = parse_rate_from_file(path)
        if rate is None:
            print(f"Could not parse success_rate from {path}")
            continue

        results.setdefault((size, assoc, lock_mode), []).append(rate)

    if not results:
        print("No valid results parsed.")
        return

    # pivot[(size, assoc)][lock_mode] = (n, avg)
    pivot = {}
    for (size, assoc, lock_mode), rates in results.items():
        avg = sum(rates) / len(rates)
        pivot.setdefault((size, assoc), {})[lock_mode] = (len(rates), avg)

    lock_modes_present = sorted({lm for _, _, lm in results.keys()})

    rows = []
    for (size, assoc), by_mode in pivot.items():
        rows.append((size, assoc, by_mode))

    def sort_key(row):
        size, assoc, _ = row
        size_bytes = SIZE_ORDER.get(size, float("inf"))
        return (size_bytes, assoc)

    rows.sort(key=sort_key)

    csv_path = out_prefix + ".csv"
    with open(csv_path, "w", newline="") as f:
        writer = csv.writer(f)
        header = ["cache_size", "assoc"]
        for lm in lock_modes_present:
            header += [f"{lm}_avg_success_rate", f"{lm}_num_runs"]
        if "locked" in lock_modes_present and "nolock" in lock_modes_present:
            header.append("nolock_minus_locked")
        writer.writerow(header)
        for size, assoc, by_mode in rows:
            row = [size, assoc]
            for lm in lock_modes_present:
                n, avg = by_mode.get(lm, (0, None))
                row += [f"{avg:.4f}" if avg is not None else "", n]
            if "locked" in lock_modes_present and "nolock" in lock_modes_present:
                locked_avg = by_mode.get("locked", (0, None))[1]
                nolock_avg = by_mode.get("nolock", (0, None))[1]
                if locked_avg is not None and nolock_avg is not None:
                    row.append(f"{nolock_avg - locked_avg:.4f}")
                else:
                    row.append("")
            writer.writerow(row)

    print(f"Wrote summary CSV to {csv_path}")

    txt_path = out_prefix + ".txt"
    with open(txt_path, "w") as f:
        col_headers = ["cache_size", "assoc"]
        for lm in lock_modes_present:
            col_headers.append(f"{lm}_avg")
        if "locked" in lock_modes_present and "nolock" in lock_modes_present:
            col_headers.append("nolock-locked")
        f.write("".join(f"{h:<16}" for h in col_headers) + "\n")
        f.write("-" * (16 * len(col_headers)) + "\n")
        for size, assoc, by_mode in rows:
            cells = [size, str(assoc)]
            for lm in lock_modes_present:
                n, avg = by_mode.get(lm, (0, None))
                cells.append(f"{avg:.4f}" if avg is not None else "n/a")
            if "locked" in lock_modes_present and "nolock" in lock_modes_present:
                locked_avg = by_mode.get("locked", (0, None))[1]
                nolock_avg = by_mode.get("nolock", (0, None))[1]
                if locked_avg is not None and nolock_avg is not None:
                    cells.append(f"{nolock_avg - locked_avg:+.4f}")
                else:
                    cells.append("n/a")
            f.write("".join(f"{c:<16}" for c in cells) + "\n")

    print(f"Wrote summary table to {txt_path}")

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not available; skipping plot. "
              "Install with: pip install matplotlib --break-system-packages")
        return

    # by_assoc[assoc][lock_mode] = list of (size_bytes, size_str, avg)
    by_assoc = {}
    for size, assoc, by_mode in rows:
        for lm, (n, avg) in by_mode.items():
            by_assoc.setdefault(assoc, {}).setdefault(lm, []).append(
                (SIZE_ORDER.get(size, 0), size, avg)
            )

    fig, ax = plt.subplots(figsize=(9, 6))

    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
    linestyles = {"locked": "-", "nolock": "--", "unlocked": ":"}

    for idx, assoc in enumerate(sorted(by_assoc.keys())):
        color = colors[idx % len(colors)]
        for lm in lock_modes_present:
            points = sorted(by_assoc[assoc].get(lm, []), key=lambda p: p[0])
            if not points:
                continue
            xs = [p[0] for p in points]
            ys = [p[2] for p in points]
            ax.plot(xs, ys, marker="o", color=color,
                     linestyle=linestyles.get(lm, "-."),
                     label=f"assoc={assoc}, {lm}")

    ax.set_xscale("log", base=2)
    ax.set_xlabel("Cache size (bytes)")
    ax.set_ylabel("Average attacker-success rate\n(fraction of secrets whose reaccess missed)")
    ax.set_title("Prime+Probe visibility vs cache size and associativity\n(locked_lru: locked vs nolock)")
    ax.set_ylim(-0.05, 1.05)
    ax.legend(title="Config", fontsize=8)
    ax.grid(True, which="both", linestyle="--", alpha=0.5)

    all_sizes = sorted(set((SIZE_ORDER.get(size, 0), size) for size, _, _ in rows))
    ax.set_xticks([s[0] for s in all_sizes])
    ax.set_xticklabels([s[1] for s in all_sizes], rotation=45, ha="right")

    fig.tight_layout()

    plot_path = out_prefix + ".png"
    fig.savefig(plot_path, dpi=150)
    print(f"Wrote plot to {plot_path}")


if __name__ == "__main__":
    main()