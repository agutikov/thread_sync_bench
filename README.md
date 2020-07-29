
TODO: scaling_governor: performance, ondemand
TODO: lost wakeup

TODO: syncronization with coroutines
https://blog.panicsoftware.com/coroutines-introduction/
https://blog.panicsoftware.com/your-first-coroutine/
https://www.modernescpp.com/index.php/c-20-coroutines-with-cppcoro
https://www.modernescpp.com/index.php/c-20-thread-synchronization-with-coroutines



Conclusion:

1. Yes, pipeline system under load shows better performance than cold one, sometimes 10x lower latency.
2. CPU frequency scaling give us x2 higher latency.
3. 
4. Pracical limit of throughput of queue synchonized with mutex and condition_variable is about **1M items/s**.
5. If you need more throughput you should group items in batches and transfer multiple items at onece.
6. Result of tests with 1-to-10 threads has no increase of latency at high throughput - this is because my CPU has 16 cores.
7. And this is only because system was not loaded - I run only tis benchmark. So CPU cores were not utilized by 100%.
8. Tests with 20-to-100 threads shows leap of latency at maximum throughput. An yes, CPU shows 100% utilization in this case.
9. When CPU is 100% then threads can't continue processing, not fetching items from queue and queue is growing. ...?
10. Optimal queue (synchronized with mutex) throughput is 10k - 100k items/s.
11. Batching is not only single way to increase throughput, I expect lockless queue can improve this case also.





