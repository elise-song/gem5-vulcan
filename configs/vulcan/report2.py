import sys


def parse_victim_accesses(first_line):
    """
    Expects the first line of the debug file to look like:
    [0x28ec5e64, 0x7200c5c, 0x199c6b4, 0x2f75903c, 0x119a1494]
    """
    inner = first_line.strip().strip("[]")
    if not inner:
        return []
    return [int(x.strip(), 16) for x in inner.split(",")]

#success_rate = fraction of secrets that yielded a miss among the victims
def check_mapping(victim_accesses, debug_file, out_file, num_accesses, num_sets):
    if len(victim_accesses) == 0:
        return 0.0

    set_mask = num_sets - 1

    total_accesses = 2 * num_accesses + 2 * num_sets
    probe_start = 2 * num_accesses + num_sets + 1
    probe_end = total_accesses

    victim_sets = [(addr >> 6) & set_mask for addr in victim_accesses]

    visible_sets = set()

    access_count = 0
    missed_addr = None

    with open(debug_file, "r") as f:
        for line in f:
            if "access for ReadReq" in line or "access for WriteReq" in line:
                access_count += 1

                if access_count > total_accesses:
                    break

                if "miss" in line:
                    bracket_split = line.split("[")[1]
                    addr_str = bracket_split.split(":")[0]
                    missed_addr = int(addr_str, 16)
                else:
                    missed_addr = None

            if (probe_start <= access_count <= probe_end
                    and missed_addr is not None
                    and "Block addr" in line and "set:" in line):

                set_split = line.split("set:")[1]
                set_value = set_split.split()[0]
                miss_set = int(set_value, 16)

                visible_sets.add(miss_set)
                missed_addr = None

    visible = [vs in visible_sets for vs in victim_sets]
    successes = sum(1 for v in visible if v)
    success_rate = successes / len(victim_accesses)

    with open(out_file, "w") as w:
        w.write('victim_accesses = [{}]\n'.format(
            ', '.join(hex(x) for x in victim_accesses)))
        w.write(
            f"num_accesses={num_accesses} num_sets={num_sets} "
            f"total_accesses={total_accesses} "
            f"probe_window=[{probe_start},{probe_end}]\n\n"
        )

        w.write("visible_sets (saw a miss during probe) = [{}]\n\n".format(
            ', '.join(hex(s) for s in sorted(visible_sets))))

        for idx, secret in enumerate(victim_accesses):
            status = "VISIBLE" if visible[idx] else "HIDDEN"
            w.write(f"secret={hex(secret)} victim_set={hex(victim_sets[idx])} "
                    f"-> {status}\n")

        w.write(f"\nsuccess_rate = {successes}/{len(victim_accesses)} = {success_rate}\n")

    return success_rate


def find_victim_accesses_line(debug_file):
    with open(debug_file, "r") as f:
        for line in f:
            if "victim_accesses" in line and "[" in line:
                #line looks like: victim_accesses = [0x28ec5e64, ...]
                bracket_part = line.split("[", 1)[1].rsplit("]", 1)[0]
                return f"[{bracket_part}]"
    raise ValueError(f"Could not find 'victim_accesses = [...]' line in {debug_file}")


if __name__ == "__main__":
    debug_file = sys.argv[1]
    out_file = sys.argv[2]
    num_accesses = int(sys.argv[3])
    num_sets = int(sys.argv[4])

    first_line = find_victim_accesses_line(debug_file)

    victim_accesses = parse_victim_accesses(first_line)

    rate = check_mapping(victim_accesses, debug_file, out_file, num_accesses, num_sets)