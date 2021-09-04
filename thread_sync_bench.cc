
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

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
            cv.wait(lock, [this]() { return !q.empty(); });
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
// 2-nd - throughput, objs/s
using frame = std::tuple<hr_clock::time_point, FrameType, double>;

using queue = sync_queue<frame>;

void wait(size_t ns)
{
    if (ns <= 100) {
        // throuhgput > 10M obj/s
        return;
    } else if (ns <= 2000) {
        // throuhgput > 500k obj/s
        volatile size_t counter = ns / 2;
        while (counter) {
            counter = counter - 1;
        }
    } else if (ns < 100000) {
        // throuhgput > 10k obj/s
        auto started = hr_clock::now();
        while ((hr_clock::now() - started) < ns * 1ns) { }
    } else {
        // throuhgput < 10k obj/s
        std::this_thread::sleep_for(ns * 1ns);
    }
}

#define MAX_BATCH_SIZE (1000 * 10)

void produce_batch(size_t throughput, std::shared_ptr<queue> sink)
{
    std::cerr << throughput << std::endl;
    size_t delay_ns = 1000'000'000 / throughput;
    size_t count = throughput;
    if (count > MAX_BATCH_SIZE) {
        count = MAX_BATCH_SIZE;
    }

    auto started = hr_clock::now();
    while (count-- > 0) {
        sink->send({hr_clock::now(), FrameType::MSG, throughput});
        wait(delay_ns);
    }
    sink->send({started, FrameType::BATCH_END, throughput});
}

void producer_worker(std::shared_ptr<queue> sink)
{
    for (size_t d1 : {10, 100, 1000, 10'000, 100'000, 1000'000}) {
        for (size_t d2 : {1, 2, 5}) {
            produce_batch(d1 * d2, sink);
            std::this_thread::sleep_for(1ms * (rand() % 1000));
        }
    }
    produce_batch(10'000'000, sink);
    std::this_thread::sleep_for(100ms);
    sink->send({hr_clock::now(), FrameType::FINISH, 0});
    std::cerr << "prod exit" << std::endl;
}

// 0-th - throughput, obj/s
// 1-nd - latency, ns
std::vector<std::tuple<double, double>> results;

// desired throughput -> resulting average throughput obj/s
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

void run_benchmark(int n_queues, const std::string& latency_filename, const std::string& throughput_filename)
{
    auto q1 = std::make_shared<queue>();

    std::thread consumer_thread(consumer_worker, q1);

    std::vector<std::thread> threads;
    std::shared_ptr<queue> src = nullptr;
    std::shared_ptr<queue> sink = q1;

    for (int i = 0; i < n_queues - 1; i++) {
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
    std::cerr << "save latency to " << latency_filename << std::endl;
    for (const auto& r : results) {
        lat_of << std::get<0>(r) << " " << std::get<1>(r) << std::endl;
    }
    results.clear();

    std::ofstream thr_of;
    thr_of.open(throughput_filename);
    std::cerr << "save throughput to " << throughput_filename << std::endl;
    for (const auto& [d, thr] : throughput) {
        thr_of << d << " " << thr << std::endl;
    }
    throughput.clear();
}

int main(int argc, const char* argv[])
{
    srand(((uint64_t)(&argc)) % 1000'000'000);

    int n_queues = strtol(argv[1], 0, 0);

    run_benchmark(n_queues, argv[2], argv[3]);

    return 0;
}
