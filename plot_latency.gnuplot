#!/usr/bin/env gnuplot

set term png enhanced size 1920,1080

set log x
set log y


set xlabel "delay between producer send (ns)"
set ylabel "latency (ns)"

set grid mxtics mytics xtics ytics lt 1 lc rgb 'gray70', lt 1 lc rgb 'gray90'

plot data, mean with linespoints ls 1 lt 7 ps 2 lc rgb 'red', median with linespoints ls 1 lt 7 ps 2 lc rgb 'blue'

#pause -1
