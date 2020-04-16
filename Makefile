
N_THREADS := 1 2 5 10 20 50 100
DATA_FILES := $(N_THREADS:%=latency_%_threads)
MEAN_FILES := $(N_THREADS:%=mean_%_threads)
PNG_FILES := $(N_THREADS:%=chart_%_threads.png)


.PHONY: clean png


thread_sync_bench: thread_sync_bench.cc
	g++ -Werror -O2 -static -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive $^ -o $@

png: $(PNG_FILES)

bench: $(DATA_FILES)

stats: $(MEAN_FILES)

$(DATA_FILES): %: thread_sync_bench
	./thread_sync_bench $(@:latency_%_threads=%) > $@

$(MEAN_FILES): mean_%_threads: latency_%_threads
	python3 ./stats.py $< $@ $(@:mean_%_threads=median_%_threads)

$(PNG_FILES): chart_%_threads.png: mean_%_threads
	gnuplot \
	-e "data='$(<:mean_%_threads=latency_%_threads)'" \
	-e "mean='$<'" \
	-e "median='$(<:mean_%_threads=median_%_threads)'" \
	./plot.gnuplot > $@

clean:
	git clean -fdx
