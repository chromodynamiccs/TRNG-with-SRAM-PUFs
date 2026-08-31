import hashlib
import math

runs = []
for line in open("C:\\Users\\Obama\\Downloads\\TRNGSRAM\\4 Cycles full chip read.txt"):
    line = line.strip()
    if ',' in line and not line.startswith('#'):
        words = line.split(',')
        bits = []
        for w in words:
            val = int(w, 16)
            for i in range(16):
                bits.append((val >> i) & 1)
        runs.append(bits)

profile_runs = runs[:-1]  
last_run     = runs[-1]   

n_bits = len(runs[0])
unstable = []
total_entropy = 0
for b in range(n_bits):
    values = [run[b] for run in profile_runs]
    p = sum(values) / len(values)            
    if 0 < p < 1:                             
        h = -math.log2(max(p, 1 - p))         
        unstable.append(b)
        total_entropy += h
        word, bit = b // 16, b % 16
        print(f"word {word:2d} bit {bit:2d}   p = {p:.2f}   H = {h:.2f}")

print(f"\nunstable bits found: {len(unstable)}")
print(f"total min-entropy: {total_entropy:.1f} bits")

bitstream = ''.join(str(last_run[b]) for b in unstable)
print(f"raw bitstream: {bitstream}")

as_bytes = int(bitstream, 2).to_bytes((len(bitstream) + 7) // 8, 'big')
random_number = hashlib.sha256(as_bytes).hexdigest()
print(f"random number: {random_number}")