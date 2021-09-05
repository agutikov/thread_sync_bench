
#include <boost/fiber/all.hpp>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#define MAX_BATCH_SIZE (100'000)

#define QUEUE_CAPACITY 1024

using namespace std::chrono_literals;
using hr_clock = std::chrono::high_resolution_clock;

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
    size_t throughput;
};

using queue = boost::fibers::buffered_channel<msg_t>;

constexpr bool produce_batches = false;

struct waiter
{
    size_t skipped_yields = 0;

    void wait(std::chrono::nanoseconds ns)
    {
        if (ns <= 1us) {
            // throuhgput >= 1M obj/s
            if constexpr (produce_batches) {
                // should act like batch processing
                // 100k batches/s
                size_t to_skip = 10'000 / (ns.count() + 1);
                if (skipped_yields++ >= to_skip) {
                    skipped_yields = 0;
                    boost::this_fiber::yield();
                }
            } else {
                boost::this_fiber::yield();
            }
        } else {
            boost::this_fiber::sleep_for(ns);
        }
    }
};

void send(queue& q, msg_t&& msg)
{
    while (true) {
        auto channel_state = q.push(msg);
        switch (channel_state) {
        case boost::fibers::channel_op_status::success:
            return;
        case boost::fibers::channel_op_status::full:
        case boost::fibers::channel_op_status::timeout:
            boost::this_fiber::yield();
            continue;
        case boost::fibers::channel_op_status::closed:
            printf("FATAL: channel closed\n");
        default:
            printf("FATAL: channel unexpected state\n");
        }
        std::this_thread::sleep_for(1s);
        std::terminate();
    }
}

msg_t recv(queue& q)
{
    msg_t msg;
    while (true) {
        auto channel_state = q.pop(msg);
        switch (channel_state) {
        case boost::fibers::channel_op_status::success:
            return msg;
        case boost::fibers::channel_op_status::empty:
        case boost::fibers::channel_op_status::timeout:
            boost::this_fiber::yield();
            continue;
        case boost::fibers::channel_op_status::closed:
            printf("FATAL: channel closed\n");
        default:
            printf("FATAL: channel unexpected state\n");
        }
        std::this_thread::sleep_for(1s);
        std::terminate();
    }
}

void produce_batch(size_t throughput, std::shared_ptr<queue> sink)
{
    std::cerr << throughput << std::endl;
    size_t delay_ns = 1000'000'000 / throughput;
    size_t count = MAX_BATCH_SIZE;
    waiter w;

    auto started = hr_clock::now();
    auto stop_before = started + 1s;
    while (hr_clock::now() < stop_before && count--) {
        send(*sink, {hr_clock::now(), FrameType::MSG, throughput});
        w.wait(delay_ns * 1ns);
    }
    send(*sink, {started, FrameType::BATCH_END, throughput});
}

void producer_worker(std::shared_ptr<queue> sink)
{
    for (size_t d1 : {10, 100, 1000, 10'000, 100'000, 1000'000, 10'000'000, 100'000'000}) {
        for (size_t d2 : {1, 2, 5}) {
            produce_batch(d1 * d2, sink);
            boost::this_fiber::sleep_for(1ms * (rand() % 1000) + 500ms);
        }
    }
    boost::this_fiber::sleep_for(200ms);
    send(*sink, {hr_clock::now(), FrameType::FINISH, 0});
    std::cerr << "prod exit" << std::endl;
}

struct result_t
{
    double throughput_obj_s;
    double latency_ns;
};

std::vector<result_t> results;

// desired throughput -> resulting average throughput obj/s
std::map<size_t, double> result_throughput;

void consumer_worker(std::shared_ptr<queue> src)
{
    size_t received = 0;
    while (true) {
        auto msg = recv(*src);
        auto stop = hr_clock::now();
        received++;
        std::chrono::duration<double, std::nano> latency = stop - msg.create_timestamp;
        if (msg.type == FrameType::MSG) {
            results.emplace_back(msg.throughput, latency.count());
        }
        if (msg.type == FrameType::BATCH_END) {
            result_throughput.emplace(msg.throughput, ((double)received) * 1000000000 / latency.count());
            received = 0;
        }
        if (msg.type == FrameType::FINISH) {
            break;
        }
    }
    std::cerr << "cons exit" << std::endl;
}

void pipe_worker(std::shared_ptr<queue> src, std::shared_ptr<queue> sink)
{
    while (true) {
        auto x = recv(*src);
        send(*sink, std::move(x));
        if (x.type == FrameType::FINISH) {
            break;
        }
    }
    std::cerr << "pipe exit" << std::endl;
}

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

thread_local static int thread_id = 0;

void use_scheduler()
{
    unsigned int thread_count = std::thread::hardware_concurrency();
    printf("thread-%d: use scheduler\n", thread_id);
    // thread registers itself at work-stealing scheduler
    boost::fibers::use_scheduling_algorithm<boost::fibers::algo::work_stealing>(thread_count);
}

static bool done = false;
static std::mutex worker_threads_mutex{};
static boost::fibers::condition_variable_any worker_threads_cv{};

void worker(uint32_t thread_count, int id)
{
    thread_id = id;

    use_scheduler();

    std::unique_lock<std::mutex> lock(worker_threads_mutex);
    worker_threads_cv.wait(lock, []() { return done; });

    printf("Exit worker thread %d function\n", thread_id);
}

std::vector<std::thread> worker_threads;

void start_scheduler_threads()
{
    unsigned int thread_count = std::thread::hardware_concurrency();
    printf("hardware concurrency: %d\n", thread_count);
    for (int i = 1; i < thread_count; i++) {
        worker_threads.emplace_back(worker, thread_count, i);
    }
}

void stop_scheduler_threads()
{
    std::unique_lock<std::mutex> lock(worker_threads_mutex);
    done = true;
    lock.unlock();
    worker_threads_cv.notify_all();

    for (auto& t : worker_threads) {
        t.join();
    }
}

bool multi_thread = true;

void run_benchmark(int n_queues, const std::string& latency_filename, const std::string& throughput_filename)
{
    auto q1 = std::make_shared<queue>(QUEUE_CAPACITY);

    boost::fibers::fiber consumer_fiber(consumer_worker, q1);

    std::vector<boost::fibers::fiber> pipe_fibers;
    std::shared_ptr<queue> src = nullptr;
    std::shared_ptr<queue> sink = q1;

    for (int i = 0; i < n_queues - 1; i++) {
        src = std::make_shared<queue>(QUEUE_CAPACITY);
        pipe_fibers.emplace_back(pipe_worker, src, sink);
        sink = src;
    }

    boost::fibers::fiber producer_fiber(producer_worker, sink);

    if (multi_thread) {
        producer_fiber.detach();
        for (auto& pipe_fiber : pipe_fibers) {
            pipe_fiber.detach();
        }
        start_scheduler_threads();
        use_scheduler();
        consumer_fiber.join();
        stop_scheduler_threads();
    } else {
        // execute all fibers in the single main thread
        producer_fiber.join();
        for (auto& pipe_fiber : pipe_fibers) {
            pipe_fiber.join();
        }
        consumer_fiber.join();
    }

    dump_latency(latency_filename);
    results.clear();

    dump_throughput(throughput_filename);
    result_throughput.clear();
}

int main(int argc, const char* argv[])
{
    srand(((uint64_t)(&argc)) % 1000'000'000);

    int n_queues = strtol(argv[1], 0, 0);

    run_benchmark(n_queues, argv[2], argv[3]);

    return 0;
}
