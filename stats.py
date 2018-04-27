
import sys
import numpy as np

m = {}

with open(sys.argv[1]) as f:
    for line in f.readlines():
        a = line.split()
        d, l = int(a[0]), int(a[1])
        if d not in m:
            m[d] = []
        m[d].append(l)

with open(sys.argv[2], 'w') as f:
    for d,v in m.items():
        s = "%d %f\n" % (d, np.mean(v))
        f.write(s)

with open(sys.argv[3], 'w') as f:
    for d,v in m.items():
        s = "%d %f\n" % (d, np.median(v))
        f.write(s)
