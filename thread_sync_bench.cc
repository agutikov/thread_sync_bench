
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <tuple>
#include <functional>
#include <map>
#include <string_view>
#include <filesystem>


namespace fs = std::filesystem;
using namespace std::chrono_literals;


template<typename T>
struct sync_queue
{
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;

    void send(T x)
    {
        std::unique_lock<std::mutex> lock(m);
        q.push(x);
        cv.notify_one();
    }

    T recv()
    {
        std::unique_lock<std::mutex> lock(m);
        if (q.empty()) {
            cv.wait(lock, [this](){ return !q.empty(); });
        }
        auto x = q.front();
        q.pop();
        return x;
    }
};


using hr_clock = std::chrono::high_resolution_clock;

enum FrameType
{
    MSG,
    BATCH_END,
    FINISH
};

// 0-th - start time_point
// 1-st - frame type
// 2-nd - frequency, objs/s
using frame = std::tuple<hr_clock::time_point, FrameType, double>;

using queue = sync_queue<frame>;


size_t min_wait_ns = 0;

void wait(size_t ns)
{
    if (ns < min_wait_ns / 2) {
        return;
    }
    if (ns > 100000) {
        std::this_thread::sleep_for(ns * 1ns);
    } else {
        auto started = hr_clock::now();
        while ((hr_clock::now() - started) < ns * 1ns) ;
    }
}

#define MAX_BATCH_SIZE 10000

void produce_batch(size_t delay, std::shared_ptr<queue> sink)
{
    std::cerr << delay << std::endl;
    size_t count = 1000000000 / (delay != 0 ? delay : 1);
    if (count > MAX_BATCH_SIZE) {
        count = MAX_BATCH_SIZE;
    }

    auto started = hr_clock::now();
    while (count-- > 0) {
        sink->send({hr_clock::now(), FrameType::MSG, delay});
        wait(delay);
    }
    sink->send({started, FrameType::BATCH_END, delay});
}

void producer_worker(std::shared_ptr<queue> sink)
{
    produce_batch(0, sink);
    for (size_t d1 : {10, 100, 1000, 10000, 100000, 1000000, 10000000}) {
        for (size_t d2 : {1, 2, 3, 4, 5, 6, 7, 8, 9}) {
            produce_batch(d1*d2, sink);
            std::this_thread::sleep_for(100ms);
        }
    }
    sink->send({hr_clock::now(), FrameType::FINISH, 0});
    std::cerr << "prod exit" << std::endl;
}

// 0-th - delay, ns
// 1-nd - latency, ns
std::vector<std::tuple<double, double>> results;

// delay -> average throughput obj/s
std::map<double, double> throughput;

void consumer_worker(std::shared_ptr<queue> src)
{
    size_t received = 0;
    while (true) {
        auto x = src->recv();
        auto stop = hr_clock::now();
        received++;
        std::chrono::duration<double, std::nano> latency = stop - std::get<0>(x);
        if (std::get<1>(x) == FrameType::MSG) {
            results.emplace_back(std::get<2>(x), latency.count());
        }
        if (std::get<1>(x) == FrameType::BATCH_END) {
            throughput.emplace(std::get<2>(x), ((double)received) * 1000000000 / latency.count());
            received = 0;
        }
        if (std::get<1>(x) == FrameType::FINISH) {
            break;
        }
    }
    std::cerr << "cons exit" << std::endl;
}

void pipe_worker(std::shared_ptr<queue> src, std::shared_ptr<queue> sink)
{
    while (true) {
        auto x = src->recv();
        sink->send(x);
        if (std::get<1>(x) == FrameType::FINISH) {
            break;
        }
    }
    std::cerr << "pipe exit" << std::endl;
}


void run_benchmark(int n_threads, fs::path latency_filename, fs::path throughput_filename)
{
    auto q1 = std::make_shared<queue>();

    std::thread consumer_thread(consumer_worker, q1);

    std::vector<std::thread> threads;
    std::shared_ptr<queue> src = nullptr;
    std::shared_ptr<queue> sink = q1;
    for (int i = 0; i < n_threads; i++) {
        src = std::make_shared<queue>();
        threads.emplace_back(pipe_worker, src, sink);
        sink = src;
    }

    std::thread producer_thread(producer_worker, sink);

    producer_thread.join();
    for (auto& t : threads) {
        t.join();
    }
    consumer_thread.join();

    std::ofstream lat_of;
    lat_of.open(latency_filename);
    for (const auto& r : results) {
        lat_of << std::get<0>(r) << " " << std::get<1>(r) << std::endl;
    }
    results.clear();

    std::ofstream thr_of;
    thr_of.open(throughput_filename);
    for (const auto& [d, thr] : throughput) {
        thr_of << d << " " << thr << std::endl;
    }
    throughput.clear();
}


int main(int argc, const char* argv[])
{
    for (int i = 0; i < 1000; i++) {
        auto t1 = hr_clock::now();
        std::chrono::duration<double, std::nano> latency = t1 - hr_clock::now();
        min_wait_ns += latency.count();
    }
    min_wait_ns /= 1000;

    int n_threads = strtol(argv[1], 0, 0);

    run_benchmark(n_threads, argv[2], argv[3]);

    return 0;
}
