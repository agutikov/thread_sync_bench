// senderReceiver.cpp

#include <atomic>
#include <chrono>
#include <coroutine>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <sstream>


using namespace std::chrono_literals;

std::string this_thread_id()
{
    auto myid = std::this_thread::get_id();
    std::stringstream ss;
    ss << myid;
    return ss.str();
}

#define TRACE() printf("%s %s\n", this_thread_id().c_str(), __PRETTY_FUNCTION__)
#define TRACE_S(MSG) printf("%s %s: %s\n", this_thread_id().c_str(), __PRETTY_FUNCTION__, MSG)
#define TRACE_F(FMT, ARGS...) printf("%s %s: " ##FMT "\n", this_thread_id().c_str(), __PRETTY_FUNCTION__, AGRS...)

class Event
{
public:
    Event()
    {
        TRACE();
    }

    ~Event()
    {
        TRACE();
    }

    // non-copyable
    Event(const Event&) = delete;
    Event(Event&&) = delete;
    // non-movable
    Event& operator=(const Event&) = delete;
    Event& operator=(Event&&) = delete;

    class Awaitable
    {
    public:
        Awaitable(const Event& eve)
            : event(eve)
        {
            TRACE();
        }

        ~Awaitable()
        {
            TRACE();
        }

        bool await_ready() const
        {
            TRACE();
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
            TRACE();
            coroutineHandle = corHandle;

            if (event.notified)
                return false;

            // store the waiter for later notification
            event.suspendedWaiter.store(this);

            return true;
        }

        void await_resume() noexcept
        {
            TRACE();
        }

    private:
        friend class Event;

        const Event& event;
        std::coroutine_handle<> coroutineHandle;
    };

    Awaitable operator co_await() const noexcept
    {
        TRACE();
        return Awaitable{*this};
    }

    void notify() noexcept
    {
        TRACE();
        notified = true;

        // try to load the waiter
        auto* waiter = static_cast<Awaitable*>(suspendedWaiter.load());

        // check if a waiter is available
        if (waiter != nullptr) {
            // resume the coroutine => await_resume
            waiter->coroutineHandle.resume();
        }
    }

private:
    friend class Awaitable;

    mutable std::atomic<void*> suspendedWaiter{nullptr};
    mutable std::atomic<bool> notified{false};
};




struct task
{
    struct promise
    {
        promise()
        {
            TRACE();
        }

        ~promise()
        {
            TRACE();
        }

        task get_return_object()
        {
            TRACE();
            return {};
        }

        std::suspend_never initial_suspend()
        {
            TRACE();
            return {};
        }

        std::suspend_never final_suspend() noexcept
        {
            TRACE();
            return {};
        }

        void return_void()
        {
            TRACE();
        }

        void unhandled_exception()
        {
            TRACE();
        }
    };

    using promise_type = promise;

    task()
    {
        TRACE();
    }

    ~task()
    {
        TRACE();
    }
};

task receiver_coro(Event& event)
{
    TRACE();
    auto start = std::chrono::high_resolution_clock::now();

    co_await event;

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Got the notification! " << std::endl;
    TRACE();
    std::cout << "Waited " << elapsed.count() * 1000'000 << " us." << std::endl;
}

void demo_async_event(std::chrono::duration<double> delay = 0ns)
{
    std::cout << std::endl;
    TRACE();

    Event event1{};
    std::jthread senderThread1;
    if (delay > 0ns) {
        std::cout << "Notification after waiting for " << delay.count() << "s" << std::endl;
        senderThread1 = std::jthread([&event1, delay] {
            TRACE_S("sender started");
            std::this_thread::sleep_for(delay);
            event1.notify();
            TRACE_S("sender finished");
        });
    } else {
        std::cout << "Notification before waiting" << std::endl;
        event1.notify();
    }
    auto receiverThread1 = std::jthread([&event1] {
        TRACE_S("receiver started");
        receiver_coro(event1);
        TRACE_S("receiver finished");
    });

    std::cout << std::endl;
}


struct awaitable
{
    std::coroutine_handle<>* coro_handle = nullptr;

    awaitable(std::coroutine_handle<>* h)
    {
        TRACE();
        coro_handle = h;
    }

    ~awaitable()
    {
        TRACE();
    }

    bool await_ready()
    {
        TRACE();
        return false;
    }

    void await_suspend(std::coroutine_handle<> h)
    {
        TRACE();
        *coro_handle = h;
    }

    void await_resume()
    {
        TRACE();
    }
};

task coro_example(std::coroutine_handle<>* h)
{
    std::cout << "Coroutine started on thread: " << std::this_thread::get_id() << '\n';
    //co_await create_awaitable();
    printf("co_await\n");
    co_await awaitable{h};
    std::cout << "Coroutine resumed on thread: " << std::this_thread::get_id() << '\n';
}

void demo_resume_on_new_thread()
{
    TRACE();

    std::coroutine_handle<> h;
    coro_example(&h);

    auto resume_thread = std::jthread([h] {
        TRACE();
        h.resume();
    });
}

int main()
{
    TRACE();
    demo_resume_on_new_thread();

    // WARNING: this is not real syncronisation of threads
    // I think this is syncronisation of coroutines
    // with 0us: event1.notify() call disables the co_await, because await_ready() returns true
    demo_async_event(0us);
    // with >0us: waiter->coroutineHandle.resume() call executes rest of receiver_coro()
    demo_async_event(0.5s);

    return 0;
}
