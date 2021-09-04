// senderReceiver.cpp

#include <atomic>
#include <chrono>
#include <coroutine>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using namespace std::chrono_literals;

class Event
{
public:
    Event() = default;

    // non-copyable
    Event(const Event&) = delete;
    Event(Event&&) = delete;
    // non-movable
    Event& operator=(const Event&) = delete;
    Event& operator=(Event&&) = delete;

    class Awaiter
    {
    public:
        Awaiter(const Event& eve)
            : event(eve)
        {
        }

        bool await_ready() const
        {
            // allow at most one waiter
            if (event.suspendedWaiter.load() != nullptr) {
                throw std::runtime_error("More than one waiter is not valid");
            }

            // event.notified == false; suspends the coroutine
            // event.notified == true; the coroutine is executed such as a usual
            // function
            return event.notified;
        }

        bool await_suspend(std::coroutine_handle<> corHandle) noexcept
        {
            coroutineHandle = corHandle;

            if (event.notified)
                return false;

            // store the waiter for later notification
            event.suspendedWaiter.store(this);

            return true;
        }

        void await_resume() noexcept { }

    private:
        friend class Event;

        const Event& event;
        std::coroutine_handle<> coroutineHandle;
    };

    Awaiter operator co_await() const noexcept { return Awaiter{*this}; }

    void notify() noexcept
    {
        notified = true;

        // try to load the waiter
        auto* waiter = static_cast<Awaiter*>(suspendedWaiter.load());

        // check if a waiter is available
        if (waiter != nullptr) {
            // resume the coroutine => await_resume
            waiter->coroutineHandle.resume();
        }
    }

private:
    friend class Awaiter;

    mutable std::atomic<void*> suspendedWaiter{nullptr};
    mutable std::atomic<bool> notified{false};
};

struct Task
{
    struct promise_type
    {
        Task get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() { }
        void unhandled_exception() { }
    };
};

Task receiver(Event& event)
{
    auto start = std::chrono::high_resolution_clock::now();
    co_await event;
    std::cout << "Got the notification! " << std::endl;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Waited " << elapsed.count() * 1000'000 << " us." << std::endl;
}

void demo_async_event(std::chrono::microseconds delay = 0us)
{
    std::cout << std::endl;

    std::cout << "Notification before waiting" << std::endl;
    Event event1{};
    std::thread senderThread1;
    if (delay > 0us) {
        senderThread1 = std::thread([&event1, delay] {
            std::this_thread::sleep_for(delay);
            event1.notify();
        });
    } else {
        senderThread1 = std::thread([&event1] { event1.notify(); });
    }
    auto receiverThread1 = std::thread(receiver, std::ref(event1));

    receiverThread1.join();
    senderThread1.join();

    std::cout << std::endl;
}

int main()
{
    demo_async_event(0us);
    demo_async_event(2s);
}
