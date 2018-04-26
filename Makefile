

.PHONY: all

all: plot.png

bench: thread_sync_bench.cc
	g++ -lpthread $^ -o $@

data.txt: bench
	./bench | tee $@

avg.txt: data.txt
	python3 ./avg.py $< | tee $@

plot.png: avg.txt
	gnuplot ./plot.gnuplot > $@
