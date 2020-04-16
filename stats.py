
import sys
import numpy as np

m = {}

median_latency = {}

with open(sys.argv[1]) as f:
    for line in f.readlines():
        a = line.split()
        d, l = float(a[0]), float(a[1])
        if d not in m:
            m[d] = []
        m[d].append(l)

with open(sys.argv[2], 'w') as f:
    for d,v in m.items():
        s = "%f %f\n" % (d, np.mean(v))
        f.write(s)

with open(sys.argv[3], 'w') as f:
    for d,v in m.items():
        m_lat = np.median(v)
        s = "%f %f\n" % (d, m_lat)
        median_latency[d] = m_lat
        f.write(s)

throughput = {}

with open(sys.argv[4]) as f:
    for line in f.readlines():
        a = line.split()
        d, t = float(a[0]), float(a[1])
        throughput[d] = t

with open(sys.argv[5], 'w') as f:
    for d in sorted(throughput.keys()):
        s = "%f %f\n" % (throughput[d], median_latency[d])
        f.write(s)

