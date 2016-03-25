# so-5.5.16: New helper function auto_join {#so_5_5_16__auto_join}

Mchains allow to use SObjectizer for writing concurrent programms even without agents. Just mchains and raw std::threads. But usage of std::thread requires some attention becase if std::thread is started as joinable thread it must be joined.

Lets see a simple case where two thread will read and process messages from a mchain:

~~~~~{.cpp}
using namespace std;
using namespace so_5;

void worker_thread(mchain_t ch)
{
    // Read all messages until ch will be closed.
    receive(from(ch), handler1, handler2, ...);
}

void sample(environment_t & env)
{
    auto ch = create_mchain(env);
    thread first_thread{worker_thread, ch};
    thread second_thread{worker_thread, ch};
    while(has_some_work())
        send<do_task>(ch, ...);
    close_retain_content(ch);
}
~~~~~

This sample has serious errors. Threads must be joined. But simple modification like:

~~~~~{.cpp}
    ...
    while(has_some_work())
        send<do_task>(ch, ...);
    close_retain_content(ch);
    second_thread.join();
    first_thread.join();
~~~~~

is not enough. Because the resulting code is not exception safe. To make it exception safe we need to do some additional work:

~~~~~{.cpp}
// Small helper class for join thread.
class auto_joiner {
    thread & thr_;
public :
    auto_joiner(thread & thr) : thr_{thr} {}
    ~auto_joiner() {
        if(thr_.joinable()) // Note this check!
            thr_.join();
    }
};
...
void sample(environment_t & env)
{
    thread first_thread, second_thread;
    auto_joiner first_joiner{first_thread}, second_joiner{second_thread};
    
    auto ch = create_mchain(env);
    // Use auto_closer feature from SO-5.5.16.
    auto ch_closer = auto_close_retain_content(ch);
    
    first_thread = thread{worker_thread, ch};
    second_thread = thread{worker_thread, ch};

    while(has_some_work())
        send<do_task>(ch, ...);
}
~~~~~

Now `sample` function is exception safe. It closes mchain automatically and joins all threads automatically.

But it requires addtional class `auto_joiner` and implementation of that class is not as simple as it seems. If you write such class the first time you probably forget to check `thr_.joinable()` condition. And your `auto_joiner` class will crash your application if thread is not started yet.

To make things simpler v.5.5.16 add helper function `so_5::auto_join`. This function allows to rewrite `sample` such way:

~~~~~{.cpp}
void sample(environment_t & env)
{
    thread first_thread, second_thread;
    auto thr_joiner = auto_join(first_thread, second_thread);
    
    auto ch = create_mchain(env);
    // Use auto_closer feature from SO-5.5.16.
    auto ch_closer = auto_close_retain_content(ch);
    
    first_thread = thread{worker_thread, ch};
    second_thread = thread{worker_thread, ch};

    while(has_some_work())
        send<do_task>(ch, ...);
}
~~~~~

