# decay_hamming_calculator.py
#
# Reads one log containing BOTH phases:
#   "# ref N"   rows -> natural cold-start captures (majority-voted into a reference)
#   "# cycle N" rows -> write-1s decay test captures
#
# Computes the Hamming distance between each test cycle and the majority-vote
# gold reference. Large distance = written 1s still surviving. Distance
# shrinking toward the floor = array has decayed back to its natural state.
#
# usage: python decay_hamming_calculator.py 1750ms_ref3000ms_comparison.txt

import sys

WORD_BITS = 16


def popcount(x):
    n = 0
    while x:
        n += x & 1
        x >>= 1
    return n


def parse(path):
    ref_rows = []
    test_rows = []
    target = None
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith("# ref"):
                ref_rows.append([])
                target = ref_rows
                continue
            if line.startswith("# cycle"):
                test_rows.append([])
                target = test_rows
                continue
            if not line or line.startswith("#") or line.startswith("="):
                continue
            words = [int(w, 16) for w in line.split(",") if w]
            if target is not None:
                target[-1] = words
    return ref_rows, test_rows


def majority_reference(ref_rows):
    # Build a per-bit majority-vote reference across all ref captures.
    n_words = len(ref_rows[0])
    ref = []
    half = len(ref_rows) / 2.0
    for w in range(n_words):
        word = 0
        for b in range(WORD_BITS):
            ones = 0
            for row in ref_rows:
                if (row[w] >> b) & 1:
                    ones += 1
            if ones > half:            # majority is 1
                word |= (1 << b)
        ref.append(word)
    return ref


def main():
    path = sys.argv[1]
    ref_rows, test_rows = parse(path)

    if not ref_rows:
        print("No '# ref' rows found -- cannot build a reference.")
        return
    if not test_rows:
        print("No '# cycle' rows found -- nothing to compare.")
        return

    ref = majority_reference(ref_rows)
    total = len(ref) * WORD_BITS

    print(f"reference: majority vote of {len(ref_rows)} cold-start reads, "
          f"{total} bits\n")
    print("cycle  hamming_dist  total_bits  dist_%")
    pcts = []
    for i, words in enumerate(test_rows):
        dist = sum(popcount(a ^ b) for a, b in zip(words, ref))
        pct = 100.0 * dist / total if total else 0.0
        pcts.append(pct)
        print(f"{i:<6} {dist:<13} {total:<11} {pct:.3f}")

    if pcts:
        print(f"\nmean dist%: {sum(pcts)/len(pcts):.3f}   "
              f"min: {min(pcts):.3f}   max: {max(pcts):.3f}   "
              f"spread: {max(pcts)-min(pcts):.3f}")
        print("\nLow dist% = decayed back to natural state.  "
              "High dist% = written 1s still surviving.")


if __name__ == "__main__":
    main()
