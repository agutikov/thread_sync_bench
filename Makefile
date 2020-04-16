
N_THREADS := 1 2 5 10 20 50 100
DATA_FILES := $(N_THREADS:%=latency_%_threads)
MEAN_FILES := $(N_THREADS:%=mean_%_threads)
PNG_FILES := $(N_THREADS:%=chart_%_threads.png)
LAT_OPS_FILES := $(N_THREADS:%=lat-ops-%-threads)


.PHONY: clean png


thread_sync_bench: thread_sync_bench.cc
	g++ -Werror -O2 -std=c++17 -static -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive $^ -o $@

png: $(PNG_FILES) lat_ops.png

bench: $(DATA_FILES)

stats: $(MEAN_FILES)

$(DATA_FILES): %: thread_sync_bench
	./thread_sync_bench $(@:latency_%_threads=%) $@ $(@:latency_%_threads=throughput_%_threads)

$(MEAN_FILES): mean_%_threads: latency_%_threads
	python3 ./stats.py $< $@ \
	$(@:mean_%_threads=median_%_threads) \
	$(@:mean_%_threads=throughput_%_threads) \
	$(@:mean_%_threads=lat-ops-%-threads)

$(PNG_FILES): chart_%_threads.png: mean_%_threads
	gnuplot \
	-e "data='$(<:mean_%_threads=latency_%_threads)'" \
	-e "mean='$<'" \
	-e "median='$(<:mean_%_threads=median_%_threads)'" \
	-p ./plot_latency.gnuplot > $@

lat_ops.png: $(MEAN_FILES)
	gnuplot -e "list='$(LAT_OPS_FILES)'" -p ./plot_latency_throughput.gnuplot > $@

clean:
	git clean -fdx
