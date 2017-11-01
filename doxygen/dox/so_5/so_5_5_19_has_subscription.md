# so-5.5.19 Checking the Presence of a Subscription {#so_5_5_19__has_subscription}

Sometimes it can be necessary to check the presense of a subscription. For example there can be an observer agent which receives messages with mboxes inside. This observer must make subscription for messages from that mbox:

~~~~~{.cpp}
struct start_listen {
    const so_5::mbox_t from_;
    ...
};
class observer : public so_5::agent_t {
    ...
    void on_start_listen(mhood_t<start_listen> cmd) {
        so_subscribe(cmd->from_).event(&observer::on_data);
    }
    void on_data(mhood_t<data> cmd) {...}
};
~~~~~

The problem is: if `start_listen` contain a mbox to which the observer is already subscribed the an exception will be throw in in `so_subscribe`.

Since v.5.5.19.5 there is a way to check presence of subscription. This check can be done by `so_5::agent_t::so_has_subscription` method:

~~~~~{.cpp}
class observer : public so_5::agent_t {
    ...
    void on_start_listen(mhood_t<start_listen> cmd) {
        // Make subscription only if there is no subscription yet.
        if(!so_has_subscription(cmd->from_, &obsever::on_data))
            so_subscribe(cmd->from_).event(&observer::on_data);
    }
    void on_data(mhood_t<data> cmd) {...}
};
~~~~~

Method `so_has_subscription` has several formats:

~~~~~{.cpp}
// Check subscription for message Msg in the default state.
so_has_subscription<Msg>(mbox);

// The same check as above but the state is specified explicitly.
so_has_subscription<Msg>(mbox, so_default_state());

// Check subscription for message which is handled by
// event-handler. Subscription is checked for the default state.
so_has_subscription(mbox, &my_agent::event_handler);

// The same check as above but the state is specified explicitly.
so_has_subscription(mbox, so_default_state(), &my_agent::event_handler);
~~~~~

Class `so_5::state_t` now also has a couple of `has_subscription` methods which allows to check the presence of a subscription:
~~~~~{.cpp}
class observer : public so_5::agent_t {
    state_t st_working{this};
    ...
    void on_start_listen(mhood_t<start_listen> cmd) {
        // Make subscription only if there is no subscription yet.
        if(!st_working.has_subscription(cmd->from_, &obsever::on_data))
            st_working.event(cmd->from_, &observer::on_data);
    }
    void on_data(mhood_t<data> cmd) {...}
};
~~~~~

