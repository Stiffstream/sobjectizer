# so-5.5.19 Mutable Messages {#so_5_5_19__mutable_messages}

# Until v.5.5.19 All Messages Were Immutable

From the very first version of SObjectizer-5 all messages inside a
SObjectizer-application were immutable. There was no possibility to receve a
message via non-const reference. Because of that all event handlers had one of
the following formats:

~~~~~{.cpp}
return_type event_handler(const message & msg);
return_type event_handler(mhood_t<message> cmd);
return_type event_handler(const mhood_t<message> & cmd);
~~~~~

The main reason for immutability was the way of message distribution: there was only one type of mbox in the first version of SObjectizer-5 -- multi-producer/multi-consumer one. It means that when a message has been sent to such mbox there can be several subscribers which can receive that message at the same time on the context of different work threads.

## When Immutability Is Not A Good Thing?

Immutability of messages is a very good thing in 1:N or N:M interaction. In such cases you can't know how many receivers will handle your message. You also can't know will some receiver redirect your message or store it to process later. Immutability of messages makes such complex scenarios possible.

But there can be cases where only 1:1 interaction is necessary. For example if agents work as a pipeline: a message goes from one agent to another and there is only one receiver for the message. In some of such cases immutability can create difficulties if a message passing through a pipeline must be modified. It can be a very big message and it is not efficient to create a modifyable copy of it for every agent in the pipeline.

### The Workaround. Very Dangerous Workaround
If all messages are immutable the only way to pass mutable value inside a message is usage of `mutable` keyword. For example, something like:
~~~~~{.cpp}
struct heavy_message : public so_5::message_t {
    mutable std::array<std::uint8_t, 10240> payload_;
    ...
};
...
class agent_in_pipeline : public so_5::agent_t {
    ...
    void on_heavy_message(mhood_t<heavy_message> cmd) {
        cmd->payload_[10] = 0xf7;
        cmd->payload_[11] = 0xf8;
        ...
    }
};
~~~~~
But this is a very dangerous approach because there is no a guarantee that the message sent will be received and processed only by one receiver. When you call `send` function like this:
~~~~~{.cpp}
so_5::send<heavy_message>(next_stage_mbox, ...);
~~~~~
you can pass MPMC mbox as `next_stage_mbox` and your message can be received by several receivers at the same time. It would lead to errors which are very hard to detect and repair.
## Real Solutions: Mutable And Immutable Messages
Since v.5.5.19 there is a real solution for such problem in SObjectizer: there are immutable messages and mutable ones. Immutable messages can be used like in previous SObjectizer's versions (moreover: all messages are still immutable by default). But there is a possibility to send a mutable message. And SObjectizer guarantees that mutable message can be sent only to the single subscriber. Thus mutable message can't be sent into MPMC mbox. Only MPSC mboxes and mchains can be used as the destination to a mutable message.

Mutable message can be redirected to another MPSC mbox or mchain. In this case mutable message behaves just like unique_ptr: you can pass it to someone else but you lose your pointer to it.

Mutable message can be converted to immutable message. But once mutable message lost its mutability and became immutable the mutability can't be returned back.
# More Details About Mutable Message
## Sending Of Mutable Messages
A message of type M can be sent as immutable message as well as mutable one. To send an instance of M as mutable message the special type wrapper `mutable_msg` must be used:
~~~~~{.cpp}
struct classical_message final : public so_5::message_t {...};
struct user_type_message final {...};

// Sending classical_message as mutable message.
so_5::send< so_5::mutable_msg<classical_message> >(dest, ...);
// Sending classical_message as immutable.
so_5::send< classical_message >(dest, ...);

// Sending user_type_message as mutable message.
so_5::send< so_5::mutable_msg<user_type_message> >(dest, ...);
// Sending user_type_message as immutable.
so_5::send< user_type_message >(dest, ...);
~~~~~
There is also another type wrapper `immutable_msg` which can be used for sending of immutable messages:
~~~~~{.cpp}
// These all are sending of immutable messages:
so_5::send< so_5::immutable_msg<classical_message> >(dest, ...);
so_5::send< classical_message >(dest, ...);
so_5::send< so_5::immutable_msg<user_type_message> >(dest, ...);
so_5::send< user_type_message >(dest, ...);
~~~~~
Please note that in `send<mutable_msg<T>>(dest,...)` the `dest` must be a MPSC mbox or mchain. Otherwise the exception will be thrown:
~~~~~{.cpp}
class some_agent : public so_5::agent_t {
public :
    ...
    virtual void so_evt_start() override {
        // Sending of mutable message into direct mbox (MPSC mbox).
        so_5::send<so_5::mutable_msg<classical_message>>(*this, ...);
        
        // Sending of mutable message into mchain.
        auto ch = create_mchain(so_environment());
        so_5::send<so_5::mutable_msg<classical_message>>(ch, ...);
        
        // An attempt to send mutable message into MPMC mbox.
        // An exception will be thrown in send().
        auto mbox = so_environment().create_mbox();
        so_5::send<so_5::mutable_msg<classical_message>>(mbox, ...); // DON'T DO THAT!!!
    }
};
~~~~~
## Receiving Of Mutable Messages
To receive a mutable message an event handler must have the following format:
~~~~~{.cpp}
return_type event_handler(so_5::mhood_t<so_5::mutable_msg<M>>);
~~~~~
or
~~~~~{.cpp}
return_type event_handler(so_5::mutable_mhood_t<M>);
~~~~~
where `M` is a message type. For example:
~~~~~{.cpp}
class demo final : public so_5::agent_t {
    struct A final : public so_5::message_t { ... };
    struct B final { ... };
public :
    ...
    virtual void so_define_agent() override {
        so_subscribe_self().event(&demo::on_A);
        so_subscribe_self().event([](mutable_mhood_t<B> cmd){...});
    }
    ...
private :
    void on_A(mhood_t<so_5::mutable_msg<A>> cmd) {...}
};
~~~~~
Note that an agent can have different event handlers for immutable and mutable message of the same type. For example:
~~~~~{.cpp}
class two_handlers final : public so_5::agent_t {
    struct M final {};
public :
    two_handlers(context_t ctx) : so_5::agent_t(std::move(ctx)) {
        so_subscribe_self()
            .event(&two_handlers::on_immutable_M)
            .event(&two_handlers::on_mutable_M);
    }
    virtual void so_evt_start() override {
        so_5::send<M>(*this); // Immutable message is sent.
        so_5::send<so_5::mutable_msg<M>>(*this); // Mutable message is sent.
    }
private :
    void on_immutable_M(mhood_t<M>) { std::cout << "on immutable" << std::endl; }
    void on_mutable_M(mhood_t<so_5::mutable_msg<M>>) { std::cout << "on mutable" << std::endl; }
};
~~~~~
## Mutable Service Requests
A mutable message can be used for service requests (e.g. for synchronous interactions). For example:
~~~~~{.cpp}
class service_provider final : public so_5::agent_t {
public :
    service_provider(context_t ctx) : so_5::agent_t(std::move(ctx)) {
        so_subscribe_self().event([](mutable_mhood_t<std::string> cmd) {
            *cmd = "<" + *cmd + ">"; // Modify incoming message.
            return std::move(*cmd); // Return modified value.
        });
    }
    ...
};
...
so_5::mbox_t provider_mbox = ...;
auto r = so_5::request_value<std::string, so_5::mutable_msg<std::string>>(
        provider_mbox, so_5::infinite_wait, "hello");
~~~~~
Note that service request with mutable message can't be sent into MPMC mbox: only MPSC mboxes or mchains can be used for `request_value` and `request_future` with mutable messages as request's parameter.
## Redirecting Of Mutable Messages
When mutable messages are used for interaction of agents in a pipeline then there is a need for redirection of the same message object to the next agent in the pipeline. Special versions of `send` functions must be used for this task:
~~~~~{.cpp}
class pipeline_part final : public so_5::agent_t {
    const so_5::mbox_t next_;
public :
    pipeline_part(context_t ctx, so_5::mbox_t next)
        : so_5::agent_t(std::move(ctx))
        , next_(std::move(next))
    {}
    ...
private :
    void event_handler(mutable_mhood_t<app_message> cmd) {
        // Modify message.
        cmd->some_data = some_new_value;
        ...
        // Redirect the message instance to next stage of the pipeline.
        so_5::send(next_, std::move(cmd));
        // NOTE: cmd is a nullptr now. It can't be used anymore.
        ...
    }
};
~~~~~
There are two things which must be mentioned:

* there is no need to specify a type of message to be redirected: a call in form `send(dest,...)` is used instead of `send<M>(dest,...)`. Type of redirected message is infered automatically;
* content of `mutable_mhood_t<M>` is moved into `send`. The source message hood object becomes a nullptr and can't be used anymore. In this case mutable message hood works like `std::unique_ptr`.

Functions `request_value` and `request_future` also have new overloads and accept message hood as a parameter. It means that received message can be redirected as service request. For example:
~~~~~{.cpp}
class pipeline_part final : public so_5::agent_t {
    const so_5::mbox_t next_;
    const so_5::mbox_t svc_provider_;
public :
    pipeline_part(context_t ctx, so_5::mbox_t next, so_5::mbox_t svc_provider)
        : so_5::agent_t(std::move(ctx))
        , next_(std::move(next))
        , svc_provider_(std::move(svc_provider))
    {}
    ...
private :
    void event_handler(mutable_mhood_t<app_message> cmd) {
        // Modify message.
        cmd->some_data = some_new_value;
        ...
        // Redirect the message instance as mutable service request.
        // Note that only result type is specified for request_value call.
        auto svc_result = so_5::request_value<processing_result>(
                svc_provider_, std::move(cmd));
        // NOTE: cmd is a nullptr now. It can't be used anymore.
        ...
    }
};
~~~~~

## Conversion Into An Immutable Message
Please note that when a mutable message is received via `mutable_mhood_t` and then redirected via `send` or `request_value/future` then redirected message will also be a mutable message. It means that redirected message can be sent only to one subscriber and can be handled only via `mutable_mhood_t`.

Sometimes it is necessary to remove mutability of a message and send the message as immutable one. It can be done via `to_immutable` helper function. For example:
~~~~~{.cpp}
void some_agent::on_some_message(mutable_mhood_t<some_message> cmd) {
    ... // Some actions with the content of cmd.
    // Now the mutable message will be resend as immutable one.
    so_5::send(another_mbox, so_5::to_immutable(std::move(cmd)));
    // NOTE: cmd is a nullptr now. It can't be used anymore.
    ...
}
~~~~~
Helper function `to_immutable` converts its argument from `mhood_t<mutable_msg<M>>` into `mhood_t<immutable_msg<M>>` and returns message hood to immutable message. This new message hood can be used as parameter for `send`, `request_value` or `receive_future`. Old mutable message hood becomes a nullptr and can't be used anymore.

Note: a mutable message can be converted to immutable message only once. An immutable message can't be converted into mutable one.
## Signals Cannot Be Mutable
Signals can't be sent nor received as mutable. It means that an attempt to write something like:
~~~~~{.cpp}
struct my_signal final : public so_5::signal_t {};
...
void some_agent::on_my_signal(mutable_mhood_t<my_signal>) {...}
...
so_5::send<so_5::mutable_msg<my_signal>>(dest);
~~~~~
will lead to compile-time errors because `mutable_msg` can't be used with signals.
## Mutable Messages And Timers
Mutable messages can't be sent as periodic messages. It means that `send_periodic` can be used with `mutable_msg` only if a `period` parameter is zero. For example:
~~~~~{.cpp}
// It is a valid call:
auto timer = so_5::send_periodic<so_5::mutable_msg<some_message>>(
    so_environment(), dest_mbox,
    std::chrono::milliseconds(200), // Delay before message appearance.
    std::chrono::milliseconds::zero(), // Period is zero.
    ...);
    
// It ins't a valid call. An exception will be thrown at run-time.
auto timer = so_5::send_periodic<so_5::mutable_msg<some_message>>(
    so_environment(), dest_mbox,
    std::chrono::milliseconds(200), // Delay before message appearance.
    std::chrono::milliseconds(150), // Period is not zero.
    ...);
~~~~~
Helper function `so_5::send_delayed` can be used for sending delayed mutable message (but destination must be MPSC mbox or mchain as usual for mutable messages).
