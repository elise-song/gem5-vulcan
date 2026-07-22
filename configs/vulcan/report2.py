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

#success_rate = fraction of secrets whose reaccess (after the prime phase)
#missed, i.e. was evicted despite (supposed) locking.

def check_mapping(victim_accesses, debug_file, out_file, num_accesses, num_sets):
    if len(victim_accesses) == 0:
        return 0.0

    set_mask = num_sets - 1
    victim_sets = [(addr >> 6) & set_mask for addr in victim_accesses]

    # addr_str -> number of "access for Read/WriteReq [addr_str:...]" lines
    # seen so far for that address. The 2nd occurrence of a victim's address
    # is its reaccess (the 1st is the initial warm-up access).
    seen_count = {}
    reaccess_missed = {}

    addr_strs = [format(addr, "x") for addr in victim_accesses]
    addr_str_to_secret = dict(zip(addr_strs, victim_accesses))

    with open(debug_file, "r") as f:
        for line in f:
            if "access for ReadReq" not in line and "access for WriteReq" not in line:
                continue

            bracket_split = line.split("[")[1]
            addr_str = bracket_split.split(":")[0]

            if addr_str not in addr_str_to_secret:
                continue

            seen_count[addr_str] = seen_count.get(addr_str, 0) + 1
            if seen_count[addr_str] == 2:
                reaccess_missed[addr_str] = "miss" in line

    visible = [reaccess_missed.get(s, False) for s in addr_strs]
    successes = sum(1 for v in visible if v)
    success_rate = successes / len(victim_accesses)

    with open(out_file, "w") as w:
        w.write('victim_accesses = [{}]\n'.format(
            ', '.join(hex(x) for x in victim_accesses)))
        w.write(f"num_accesses={num_accesses} num_sets={num_sets}\n\n")

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

    rate = check_mapping(victim_accesses, debug_file, out_file, num_accesses,
                          num_sets)