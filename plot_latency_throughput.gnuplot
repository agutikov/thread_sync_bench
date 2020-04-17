#!/usr/bin/env gnuplot

set term png enhanced size 1920,1080

set log x
set log y

set key left top

set xlabel "througput (obj/s)"
set ylabel "mean latency (ns)"

set grid mxtics mytics xtics ytics lt 1 lc rgb 'gray70', lt 1 lc rgb 'gray90'

plot for [file in list] file with linespoint lw 2 ps 2 title file

#pause -1
