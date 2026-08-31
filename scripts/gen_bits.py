import random
import os

f = open(os.path.join(os.path.dirname(__file__), '..', 'data', 'raw', 'random_bits.txt'), "w")
for i in range(183356):
    f.write(str(random.randint(0, 1)))
f.close()

print("done")
