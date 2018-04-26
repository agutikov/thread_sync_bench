#!/usr/bin/env gnuplot

set grid mxtics mytics xtics ytics lt 1 lc rgb 'gray70', lt 1 lc rgb 'gray90'

plot 'data.txt', 'avg.txt' lt 7
