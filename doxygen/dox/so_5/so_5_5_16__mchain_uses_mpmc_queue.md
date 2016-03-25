# so-5.5.16: mchains use MPMC queue inside {#so_5_5_16__mchains_are_mpmc}

Since v.5.5.16 mchains use Multi-Producer/Multi-Consumer queue inside. It makes possible to use the same mchain in several parallel `receive` on different threads. It could be used for simple load balancing scheme, for example:

~~~~~{.cpp}
void worker_thread(so_5::mchain_t ch)
{
    // Handle all messages until mchain will be closed.
    receive(from(ch), handler1, handler2, handler3, ...);
}
...
// Message chain for passing messages to worker threads.
auto ch = create_mchain(env);
// Worker thread.
thread worker1{worker_thread, ch};
thread worker2{worker_thread, ch};
thread worker3{worker_thread, ch};
// Send messages to workers.
while(has_some_work())
{
  so_5::send< some_message >(ch, ...);
  ...
}
// Close chain and finish worker threads.
close_retain_content(ch);
worker3.join();
worker2.join();
worker1.join();
~~~~~
