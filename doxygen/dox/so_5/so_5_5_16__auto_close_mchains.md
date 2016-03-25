# so-5.5.16: New helper function auto_close_mchains {#so_5_5_16__auto_close_mchains}

New helper function `so_5::auto_close_mchains` has been added in v.5.5.16. It helps to close some mchains at scope exit.

When mchains are used for inter thread communication special care must be taken because there could be deadlocks. For example let see the simplest case:

~~~~~{.cpp}
using namespace std;
using namespace so_5;
void do_something(environment_t & env)
{
    auto ch = create_mchain(env);
    // Launch child thread which will read messages and do some processing.
    thread child{ [ch]{
         // Return from receive will be on close of ch.
        receive(from(ch), [](const some_task & t) {...});
    } };
    // Do generation of tasks for child thread.
    while(has_tasks())
    {
        ... // generation of next task.
        send<some_task>(ch, ...);
    }
    // Child thread must be joined before return from do_something.
    child.join();
}
~~~~~

This is an error in this example: `do_something` will not return. It is becase child thread will never finished. Because mchain `ch` will not be closed explicitely.

It easy to fix that example:

~~~~~{.cpp}
    // Do generation of tasks for child thread.
    while(has_tasks())
    {
        ... // generation of next task.
        send<some_task>(ch, ...);
    }
    // Mchain must be closed to force finish of child thread.
    close_retain_content(ch);
    // Child thread must be joined before return from do_something.
    child.join();
~~~~~

But there could be more complex cases related to exception safety. If an exception is thrown after construction of `child` but before call to `close_retain_content` there could be a deadlock (see above).

To make this code exception safe we need two helpers: one of them should close chain, another should call join for threads. Something like that:

~~~~~{.cpp}
class auto_closer {
    mchain_t & ch_;
public :
    auto_closer(mchain_t & ch) : ch_{ch} {}
    ~auto_closer() { close_retain_content(ch_); }
};
class auto_joiner {
    thread & thr_;
public :
    auto_joiner(thread & thr) : thr_{thr} {}
    ~auto_joiner() { if(thr_.joinable()) thr_.close(); }
};
~~~~~

With the help of these classes we can make `do_something` more exception safe:

~~~~~{.cpp}
void do_something(environment_t & env)
{
    auto ch = create_mchain(env);
    // Launch child thread which will read messages and do some processing.
    thread child{ [ch]{
         // Return from receive will be on close of ch.
        receive(from(ch), [](const some_task & t) {...});
    } };
    // There is a trick: auto_joiner must be created before auto_closer.
    // It is because we need the destructor of auto_closer be called first.
    auto_joiner child_joiner{ child };
    auto_closer ch_closer{ ch };
    
    // Do generation of tasks for child thread.
    while(has_tasks())
    {
        ... // generation of next task.
        send<some_task>(ch, ...);
    }
    // There is no need for calling close() and join() anymore.
}
~~~~~

But writting ad-hoc classes like `auto_closer` is a boring and error-prone tasks. Helper function `so_5::auto_close_mchains` do it for you:

~~~~~{.cpp}
void do_something(environment_t & env)
{
    auto ch = create_mchain(env);
    // Launch child thread which will read messages and do some processing.
    thread child{ [ch]{
         // Return from receive will be on close of ch.
        receive(from(ch), [](const some_task & t) {...});
    } };
    auto_joiner child_joiner{ child };
    auto ch_closer = auto_close_mchains(mchain_props::close_mode_t::retain_content, ch);
    
    // Do generation of tasks for child thread.
    while(has_tasks())
    {
        ... // generation of next task.
        send<some_task>(ch, ...);
    }
    // There is no need for calling close() and join() anymore.
}
~~~~~

Function `auto_close_mchains` returns object very similar to `auto_closer` shown above. The destructor of that object will close all mchains passed to `auto_close_mchains`. It is possible to pass several mchains to one call of `auto_close_mchains`:

~~~~~{.cpp}
auto command_ch = create_mchain(env);
auto reply_ch = create_mchain(env);
auto status_ch = create_mchain(env);
auto log_ch = create_mchain(env);
auto ch_closer = auto_close_mchains(mchain_props::close_mode_t::drop_content,
    command_ch, reply_ch, status_ch, log_ch);
...
~~~~~

Because usage of `auto_close_mchains` is verbose there are two shorthands: `auto_close_retain_content` and `auto_close_drop_content`. So instead of writting:

~~~~~{.cpp}
auto ch_closer = auto_close_mchains(mchain_props::close_mode_t::drop_content,
    command_ch, reply_ch, status_ch, log_ch);
~~~~~

it is possible to write:

~~~~~{.cpp}
auto ch_closer = auto_close_drop_content(command_ch, reply_ch, status_ch, log_ch);
~~~~~
