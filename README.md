



Conclusion:

1. Yes, pipeline system under load shows better performance than cold one, sometimes 10x lower latency.
2. CPU frequency scaling give us x2 higher latency.
3. Pracical limit of throughput of queue synchonized with mutex and condition_variable is about **1M items/s**.
4. If you need more throughput you should group items in batches and transfer multiple items at onece.
5. Result of tests with 1-to-10 threads has no increase of latency at high throughput - this is because my CPU has 16 cores.
6. And this is only because system was not loaded - I run only tis benchmark. So CPU cores were not utilized by 100%.
7. Tests with 20-to-100 threads shows leap of latency at maximum throughput. An yes, CPU shows 100% utilization in this case.
8. When CPU is 100% then threads can't continue processing, not fetching items from queue and queue is growing. ...?
9.  Optimal queue (synchronized with mutex) throughput is 10k - 100k items/s.
10. Batching is not only single way to increase throughput, I expect lockless queue can improve this case also.

