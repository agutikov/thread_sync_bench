
#include <boost/fiber/all.hpp>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <tuple>
#include <vector>

#define MAX_BATCH_SIZE (1000 * 10)

using namespace std::chrono_literals;
using hr_clock = std::chrono::high_resolution_clock;

using namespace boost::fibers;
using namespace boost::this_fiber;

enum FrameType
{
    MSG,
    BATCH_END,
    FINISH
};

struct msg_t
{
    hr_clock::time_point create_timestamp;
    FrameType type;
    double throughput;
};

struct result_t
{
    double throughput_obj_s;
    double latency_ns;
};

std::vector<result_t> results;

// desired throughput -> resulting average throughput obj/s
std::map<double, double> result_throughput;

void dump_latency(const std::string& filename)
{
    std::ofstream lat_of;
    lat_of.open(filename);
    std::cerr << "save latency to " << filename << std::endl;
    for (const auto& r : results) {
        lat_of << r.throughput_obj_s << " " << r.latency_ns << std::endl;
    }
}

void dump_throughput(const std::string& filename)
{
    std::ofstream thr_of;
    thr_of.open(filename);
    std::cerr << "save result throughput table to " << filename << std::endl;
    for (const auto& [d, thr] : result_throughput) {
        thr_of << d << " " << thr << std::endl;
    }
}

void run_benchmark(int n_threads, const std::string& latency_filename, const std::string& throughput_filename)
{

    dump_latency(latency_filename);
    results.clear();

    dump_throughput(throughput_filename);
    result_throughput.clear();
}

int main(int argc, const char* argv[])
{
    srand(((uint64_t)(&argc)) % 1000000000);

    fiber f1([]() {
        printf("Hello Fiber\n");
    });

    return 0;

    int n_threads = strtol(argv[1], 0, 0);

    run_benchmark(n_threads, argv[2], argv[3]);

    return 0;
}
