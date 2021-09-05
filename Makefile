
N_QUEUES := 1 1000
BENCH := boost_fiber_bench

DATA_FILES := $(N_QUEUES:%=latency_%_queues.csv)
MEAN_FILES := $(N_QUEUES:%=mean_%_queues.csv)
PNG_FILES := $(N_QUEUES:%=chart_%_queues.png)
# following names are with minuses instead of underscores because of gnuplot
MEDIAN_LAT_OPS_FILES := $(N_QUEUES:%=median-lat-ops-%-queues.csv)
MEAN_LAT_OPS_FILES := $(N_QUEUES:%=mean-lat-ops-%-queues.csv)

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

report: $(PNG_FILES) median_lat_ops.png mean_lat_ops.png

bench: $(DATA_FILES)

$(DATA_FILES): %: $(BENCH)
#	numactl --cpunodebind=0 --membind=0 --
	./$(BENCH) $(@:latency_%_queues.csv=%) $@ \
	$(@:latency_%_queues.csv=mean_%_queues.csv) \
	$(@:latency_%_queues.csv=median_%_queues.csv) \
	$(@:latency_%_queues.csv=mean-lat-ops-%-queues.csv) \
	$(@:latency_%_queues.csv=median-lat-ops-%-queues.csv)

$(PNG_FILES): chart_%_queues.png: mean_%_queues.csv
	gnuplot \
	-e "data='$(<:mean_%_queues.csv=latency_%_queues.csv)'" \
	-e "mean='$<'" \
	-e "median='$(<:mean_%_queues.csv=median_%_queues.csv)'" \
	-p ./plot_latency.gnuplot > $@

median_lat_ops.png: $(MEAN_FILES)
	gnuplot -e "list='$(MEDIAN_LAT_OPS_FILES)'" -p ./plot_latency_throughput.gnuplot > $@

mean_lat_ops.png: $(MEAN_FILES)
	gnuplot -e "list='$(MEAN_LAT_OPS_FILES)'" -p ./plot_latency_throughput.gnuplot > $@

clean:
	git clean -fdx
