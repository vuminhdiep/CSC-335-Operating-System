# Questions for Matt: 
I intend to ask you on Friday at 5:30pm on April 28 but you already left so when you see this can you answer them in your grading feedback?

- What is random_yield(4) and spinlock besides semaphores created in synchtest.c for?
- How can I check for the producer and consumer if the number of producers and consumers are not equal? How do I check for the buffer size is empty or full to raise panic?
- Why does putting print function in the thread when testing gives out different result than putting print function in the producer and consumer itself? (answered below in additional notes but I want to know your thoughts on my answer)

# Additional Notes
- My shared buffer test seems to work when the producer and consumer threads are equal and the next step would be causing panic when the buffer size is full and trying to produce or buffer size is empty and trying to consume. I put in a KASSERT for when number of the producer is less than or equal to the number of consumer then the buffer counter would be 0.
- When I change the producer thread to be < consumer thread then the buffer will have extra items that are not consumed but when I change the consumer thread to be > producer thread, there are less items so the program doesn't exit and the consumer go to the wait channel. I think the next step would be to causing exception when this happens.
- The reason why I put kprintf the consumed and produced item in the producer and consumer function in `shared_buffer.c` instead of the threads in `shared_buffer_test.c` because in the producer and consumer have lock so it is guranteed that each producer is run sequentially whereas in the testing file the threads are atomic and can be run in different order between each producer or consumer
- I realized I wrote 4 interleaving files but yielded 6 locations because I found 2 locations in `linked_list_prepend` and `linked_list_remove_head` and I put them in the same file. I also added an extra file called `interleaving_output_with_lock.txt` to test those interleavings after using lock to prove that lock is correctly implemented.

