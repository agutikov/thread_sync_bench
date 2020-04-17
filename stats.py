
import sys
import numpy as np

m = {}

median_latency = {}

with open(sys.argv[1]) as f:
    for line in f.readlines():
        a = line.split()
        key, l = float(a[0]), float(a[1])
        if key not in m:
            m[key] = []
        m[key].append(l)

with open(sys.argv[2], 'w') as f:
    for key, v in m.items():
        s = "%f %f\n" % (key, np.mean(v))
        f.write(s)

with open(sys.argv[3], 'w') as f:
    for key, v in m.items():
        m_lat = np.median(v)
        s = "%f %f\n" % (key, m_lat)
        median_latency[key] = m_lat
        f.write(s)

throughput = {}

with open(sys.argv[4]) as f:
    for line in f.readlines():
        a = line.split()
        key, t = float(a[0]), float(a[1])
        throughput[key] = t

with open(sys.argv[5], 'w') as f:
    for key in sorted(throughput.keys()):
        s = "%f %f\n" % (throughput[key], median_latency[key])
        f.write(s)

