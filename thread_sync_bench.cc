
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <vector>
#include <memory>
#include <tuple>
#include <functional>


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

// 0-th - start time_point
// 1-st - end mark (true - continue)
// 2-nd - frequency, objs/s
using frame = std::tuple<hr_clock::time_point, bool, double>;

using queue = sync_queue<frame>;


size_t count_from_delay(size_t delay_ns)
{
    if (delay_ns <= 10000) {
        return 1000;
    }
    if (delay_ns >= 1000000) {
        return 10;
    }
    return 100000000 / delay_ns;
}

double freq_from_delay(size_t delay_ns)
{
    return 1000000000.0 / delay_ns;
}

void produce_batch(size_t delay, std::shared_ptr<queue> sink)
{
    std::cerr << delay << std::endl;
    size_t count = count_from_delay(delay);
    while (count-- > 0) {
        if (delay > 0) {
            std::this_thread::sleep_for(delay * 1ns);
        }
        sink->send({hr_clock::now(), true, freq_from_delay(delay)});
    }
}

void producer_worker(std::shared_ptr<queue> sink)
{
    produce_batch(0, sink);
    for (size_t d1 : {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000}) {
        for (size_t d2 : {1, 2, 3, 4, 5, 6, 7, 8, 9}) {
            produce_batch(d1*d2, sink);
            std::this_thread::sleep_for(1s);
        }
    }
    sink->send({hr_clock::now(), false, 0});
    std::cerr << "prod exit" << std::endl;
}

// 0-th - ops/s
// 1-nd - latency, us
std::vector<std::tuple<double, double>> results;

void consumer_worker(std::shared_ptr<queue> src)
{
    while (true) {
        auto x = src->recv();
        if (!std::get<1>(x)) {
            break;
        }
        auto stop = hr_clock::now();
        std::chrono::duration<double, std::micro> latency = stop - std::get<0>(x);
        results.emplace_back(freq_from_delay(std::get<2>(x)), latency.count());
    }
    std::cerr << "cons exit" << std::endl;
}

void pipe_worker(std::shared_ptr<queue> src, std::shared_ptr<queue> sink)
{
    while (true) {
        auto x = src->recv();
        sink->send(x);
        if (!std::get<1>(x)) {
            break;
        }
    }
    std::cerr << "pipe exit" << std::endl;
}


void run_benchmark(int n_threads)
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

    for (const auto& r : results) {
        std::cout << std::get<0>(r) << " " << std::get<1>(r) << std::endl;
    }
    results.clear();
}


int main(int argc, const char* argv[])
{
    int n_threads = strtol(argv[1], 0, 0);

    run_benchmark(n_threads);

    return 0;
}
