
N_THREADS := 1 2 10 100 1000
BENCH := boost_fiber_bench

DATA_FILES := $(N_THREADS:%=latency_%_threads)
MEAN_FILES := $(N_THREADS:%=mean_%_threads)
PNG_FILES := $(N_THREADS:%=chart_%_threads.png)
LAT_OPS_FILES := $(N_THREADS:%=lat-ops-%-threads)

CXX=g++-11

#CXX_FLAGS=-Werror -O0 -ggdb -std=c++20 -lpthread -pthread -fcoroutines
CXX_FLAGS=-Werror -O2 -std=c++20 -lpthread -pthread -fcoroutines

# -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
BOOST_LIBS=-lboost_fiber -lboost_context
LIBS=-ltcmalloc

all: thread_sync_bench coro_samples boost_fiber_bench

coro_samples: coro_samples.cc
	$(CXX) $(CXX_FLAGS) $^ -o $@ $(LIBS)

thread_sync_bench: thread_sync_bench.cc
	$(CXX) $(CXX_FLAGS)  $^ -o $@ $(LIBS)

boost_fiber_bench: boost_fiber_bench.cc
	$(CXX) $(CXX_FLAGS) $^ -o $@ $(LIBS) $(BOOST_LIBS)


png: $(PNG_FILES) lat_ops.png

bench: $(DATA_FILES)

stats: $(MEAN_FILES)

$(DATA_FILES): %: thread_sync_bench
#	numactl --cpunodebind=0 --membind=0 --
	./$(BENCH) $(@:latency_%_threads=%) $@ $(@:latency_%_threads=throughput_%_threads)

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
