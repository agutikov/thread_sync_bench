#!/usr/bin/env gnuplot

set term png enhanced size 960,540

set log x
set log y

set grid mxtics mytics xtics ytics lt 1 lc rgb 'gray70', lt 1 lc rgb 'gray90'

plot 'data.txt', 'avg.txt' lt 7

#pause -1
