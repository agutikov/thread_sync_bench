
import sys

m = {}

with open(sys.argv[1]) as f:
    for line in f.readlines():
        a = line.split()
        d, l = int(a[0]), int(a[1])
        if d not in m:
            m[d] = [0, 0]
        m[d][0] += l;
        m[d][1] += 1;

for d,v in m.items():
    print(d, v[0] / v[1])
