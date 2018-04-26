#!/bin/bash

g++ -lpthread thread_sync_bench.cc -o thread_sync_bench

./thread_sync_bench | tee data.txt

python3 ./avg.py data.txt | tee avg.txt

gnuplot ./plot.gnuplot
