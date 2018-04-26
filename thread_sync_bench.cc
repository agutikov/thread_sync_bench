
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <vector>

using namespace std::chrono_literals;

using hr_clock = std::chrono::high_resolution_clock;

// 1sr - start time_point,
// 2nd - end mark (true - continue),
// 3rd - delay in us
using frame_t = std::tuple<hr_clock::time_point, bool, int64_t>;

struct sync_queue
{
  std::queue<frame_t> q;
  std::mutex m;
  std::condition_variable cv;

  void send(frame_t x)
  {
    std::unique_lock<std::mutex> lock(m);
    q.push(x);
    cv.notify_one();
  }

  frame_t recv()
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

void producer_worker(sync_queue& sink)
{
  for (int64_t d1 : {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000}) {
    for (int64_t d2 : {1, 2, 5}) {
      int64_t delay = d1*d2;
      std::cerr << delay << std::endl;
      std::this_thread::sleep_for(1s);
      int cnt = 100;
      while (cnt-- > 0) {
        if (delay > 0) {
          std::this_thread::sleep_for(delay * 1ns);
        }
        sink.send({hr_clock::now(), true, delay});
      }
    }
  }
  std::cerr << "prod exit" << std::endl;
}

std::vector<std::tuple<int64_t, hr_clock::duration>> results;

void consumer_worker(sync_queue& src)
{
  while (true) {
    auto x = src.recv();
    if (!std::get<1>(x)) {
      break;
    }
    auto stop = hr_clock::now();
    results.emplace_back(std::get<2>(x), stop - std::get<0>(x));
  }
  std::cerr << "cons exit" << std::endl;
}

void pipe_worker(sync_queue& src, sync_queue& sink)
{
  while (true) {
    auto x = src.recv();
    sink.send(x);
    if (!std::get<1>(x)) {
      break;
    }
  }
  std::cerr << "pipe exit" << std::endl;
}

int main(int argc, const char* argv[])
{
  sync_queue q1;

  std::thread consumer_thread(consumer_worker, std::ref(q1));

  std::vector<std::thread> threads;
  sync_queue* src = nullptr;
  sync_queue* sink = &q1;
  for (int i = 0; i < 99; i++) {
    src = new sync_queue();
    threads.emplace_back(pipe_worker, std::ref(*src), std::ref(*sink));
    sink = src;
  }

  std::thread producer_thread(producer_worker, std::ref(*sink));

  producer_thread.join();
  for (auto& t : threads) {
    t.join();
  }
  consumer_thread.join();

  for (const auto& r : results) {
    std::cout << std::get<0>(r) << " " << std::get<1>(r)/1ns << std::endl;
  }

  return 0;
}
