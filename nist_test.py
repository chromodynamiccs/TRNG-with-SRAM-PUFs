import numpy as np
from nistrng import *

bits = open("C:\\Users\\Obama\\Downloads\\TRNGSRAM\\random_bits.txt").read()
print(len(bits))
seq = np.array([int(b) for b in bits], dtype=np.int64)

results = run_all_battery(seq, check_eligibility_all_battery(seq, SP800_22R1A_BATTERY), False)

for result, elapsed in results:
    print(result.name, result.score, result.passed)