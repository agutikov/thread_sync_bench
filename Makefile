
N_QUEUES := 1 1000
BENCH := boost_fiber_bench

DATA_FILES := $(N_QUEUES:%=latency_%_queues.csv)
MEAN_FILES := $(N_QUEUES:%=mean_%_queues.csv)
PNG_FILES := $(N_QUEUES:%=chart_%_queues.png)
LAT_OPS_FILES := $(N_QUEUES:%=lat-ops-%-queues.csv)

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
	$(CXX) $(CXX_FLAGS) $^ -o $@ $(LIBS)

boost_fiber_bench: boost_fiber_bench.cc
	$(CXX) $(CXX_FLAGS) $^ -o $@ $(LIBS) $(BOOST_LIBS)


png: $(PNG_FILES) lat_ops.png

bench: $(DATA_FILES)

stats: $(MEAN_FILES)

$(DATA_FILES): %: $(BENCH)
#	numactl --cpunodebind=0 --membind=0 --
	./$(BENCH) $(@:latency_%_queues.csv=%) $@ $(@:latency_%_queues.csv=throughput_%_queues.csv)

$(MEAN_FILES): mean_%_queues.csv: latency_%_queues.csv
	python3 ./stats.py $< $@ \
	$(@:mean_%_queues.csv=median_%_queues.csv) \
	$(@:mean_%_queues.csv=throughput_%_queues.csv) \
	$(@:mean_%_queues.csv=lat-ops-%-queues.csv)

$(PNG_FILES): chart_%_queues.png: mean_%_queues.csv
	gnuplot \
	-e "data='$(<:mean_%_queues.csv=latency_%_queues.csv)'" \
	-e "mean='$<'" \
	-e "median='$(<:mean_%_queues.csv=median_%_queues.csv)'" \
	-p ./plot_latency.gnuplot > $@

lat_ops.png: $(MEAN_FILES)
	gnuplot -e "list='$(LAT_OPS_FILES)'" -p ./plot_latency_throughput.gnuplot > $@

clean:
	git clean -fdx
