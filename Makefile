

.PHONY: all stats

all: plot.png

bench: thread_sync_bench.cc
	g++ -lpthread $^ -o $@

data.txt: bench
	./bench | tee $@

stats: data.txt
	python3 ./stats.py $< mean.txt median.txt

plot.png: stats
	gnuplot ./plot.gnuplot > $@
