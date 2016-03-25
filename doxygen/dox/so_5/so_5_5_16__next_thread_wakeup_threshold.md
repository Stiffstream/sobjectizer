# so-5.5.16: New next_thread_wakeup_threshold param for thread-pool dispatchers {#so_5_5_16__next_thread_wakeup_threshold}

Until v.5.5.16 thread-pool dispatchers (`thread_pool` and `adv_thread_pool`) used very simple working scheme: when new message was stored in demand queue and there was some sleeping worker thread this sleeping thread was woken up. This scheme works well if message processing is long action. In that case the cost of wakening of sleeping thread in much lower than cost of message processing.

But there could be scenarios where message processing is very quick and cheep. In such cases can be more efficient not to wake up a sleeping working thread but to allow to hold a message in demand queue for some time. Lets see small example:

~~~~~{.cpp}
using namespace so_5;
// The first agent in message handling chain. Do parameters checking.
class params_checker final : public agent_t
{
    const mbox_t m_next;
public :
    params_checker(context_t ctx, mbox_t next) : agent_t{ctx}, m_next{move(next)}
    {
        so_subscribe_self().event([this](const initial_request & r) {
            if(valid_request(r))
                // Send request to the next stage.
                send<checked_request>(m_next, r);
        });
        ...
    }
    ...
};
// Next agent in message handling chain. Do some parameters transformation.
class params_transformer final : public agent_t
{
    const mbox_t m_next;
public :
    params_transformer(context_t ctx, mbox_t next) : agent_t{ctx}, m_next{move(next)}
    {
        so_subscribe_self().event([this](const checked_request & r) {
            send<transformed_request>(m_next, transform(r));
        });
        ...
    }
    ...
};
~~~~~

Suppose that `params_checker` and `params_transformer` are part of one coop and are bound to `thread_pool` dispatcher. When `params_checker` sends `checked_request` message to `params_transformer` it might be better to keep this message in demand queue for some time and handle in on the same working thread after return from `params_checker`s event handler.

Since v.5.5.16 there is such parameter as `next_thread_wakeup_threshold` for `thread_pool` and `adv_thread_pool` dispatchers.

This paraments holds a value of demand queue size. When actual demand queue size of dispatcher becomes greater than `next_thread_wakeup_threshold` then a sleeping working thread will be woken up (if there is any sleeping thread for the dispatcher).

By default `next_thread_wakeup_threshold` is 0. It means that `thread_pool` and `adv_thread_pool` works like in previous versions. But this value can be changed. For example:

~~~~~{.cpp}
using namespace so_5;
using namespace so_5::disp::thread_pool;
environment_t & env = ...;
auto disp = create_private_disp(
    env,
    "my-thread-pool",
    disp_params_t{}
        .thread_count( 16 )
        .tune_queue_params(
            []( queue_traits::queue_params_t & qp ) {
                // A sleeping thread will be woken up if there are 3 or more
                // demands in the dispatcher's demands queue.
                qp.next_thread_wakeup_threshold( 2 );
            } )
);
~~~~~

