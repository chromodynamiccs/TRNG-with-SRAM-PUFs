import random

f = open("C:\\Users\\Obama\\Downloads\\TRNGSRAM\\random_bits.txt", "w")
for i in range(183356):
    f.write(str(random.randint(0, 1)))
f.close()

print("done")
