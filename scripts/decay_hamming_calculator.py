# decay_from_gold.py -- flip fraction (Hamming distance) vs gold reference, per cycle
#
# usage: python decay_from_gold.py decay_ones_1750.txt 1
#        python decay_from_gold.py decay_zeros_1750.txt 0
#
#   arg 1: the captured Serial Monitor log file
#   arg 2: 1 if you wrote all-1s (0xFFFF), 0 if you wrote all-0s (0x0000)
#
# The parser skips header/footer ("=== ... ==="), "# cycle N" markers, and blank
# lines. Only comma-separated hex data rows are counted, so you can save the raw
# Serial Monitor dump as-is.

import sys


def popcount(x):
    n = 0
    while x:
        n += x & 1
        x >>= 1
    return n


def main():
    path = sys.argv[1]
    gold_is_ones = (sys.argv[2] == "1") if len(sys.argv) > 2 else True

    cycles = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith("# cycle"):
                cycles.append([])
                continue
            if not line or line.startswith("#") or line.startswith("="):
                continue
            # data row: comma-separated hex words, all on one line
            words = [int(w, 16) for w in line.split(",") if w]
            if cycles:
                cycles[-1] = words

    print("cycle  flipped_bits  total_bits  flip_%")
    pcts = []
    for i, words in enumerate(cycles):
        total = len(words) * 16
        if gold_is_ones:
            flipped = sum(16 - popcount(w) for w in words)   # bits that fell 1->0
        else:
            flipped = sum(popcount(w) for w in words)         # bits that rose 0->1
        pct = 100.0 * flipped / total if total else 0.0
        pcts.append(pct)
        print(f"{i:<6} {flipped:<13} {total:<11} {pct:.3f}")

    if pcts:
        print(f"\nmean flip%: {sum(pcts)/len(pcts):.3f}   "
              f"min: {min(pcts):.3f}   max: {max(pcts):.3f}   "
              f"spread: {max(pcts)-min(pcts):.3f}")


if __name__ == "__main__":
    main()
