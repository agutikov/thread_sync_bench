

.PHONY: all stats clean

all: plot.png

bench: thread_sync_bench.cc
	g++ -Werror -O2 -static -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive $^ -o $@

data.txt: bench
	./bench > $@

stats: data.txt
	python3 ./stats.py $< mean.txt median.txt

plot.png: stats
	gnuplot ./plot.gnuplot > $@

clean:
	git clean -fdx
