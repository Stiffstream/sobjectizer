# so-5.5.19 New Format of Send Functions to Simplify Message Redirection {#so_5_5_19__new_format_of_send_functions}

New overloads for `send`, `send_delayed`, `send_periodic`, `request_future` and
`request_value` have been added in v.5.5.19. These overloads simplify
redirection of messages. They accept `mhood_t<M>`, `mhood_t<immutable_msg<M>>`
and `mhood_t<mutable_msg<M>>` arguments. For example:

~~~~~{.cpp}
class demo final : public so_5::agent_t {
    ...
    void first_event_handler(mhood_t<first> cmd) {
        // Redirection of immutable message.
        // After call to `send` cmd still contains valid pointer to the message.
        so_5::send(another_mbox, cmd);
    }
    void second_event_handler(mhood_t<so_5::immutable_message<second>> cmd) {
        // Redirection of immutable message as delayed message.
        // After call to `send` cmd still contains valid pointer to the message.
        so_5::send_delayed(so_environment(), another_mbox, std::chrono::milliseconds(100), cmd);
    }
    void third_event_handler(mhood_t<third> cmd) {
        // Redirection of immutable message as periodic message.
        // After call to `send` cmd still contains valid pointer to the message.
        so_5::send_periodic(so_environment(), another_mbox,
                std::chrono::milliseconds(100), std::chrono::milliseconds(150),
                cmd);
    }
    void forth_event_handler(mutable_mhood_t<forth> cmd) {
        // Redirection of mutable message.
        // After call to `send` cmd becomes a nullptr.
        so_5::send(another_mbox, std::move(cmd));
    }
    void fifth_event_handler(mutable_mhood_t<fifth> cmd) {
        // Redirection of mutable message as delayed message.
        // After call to `send` cmd becomes a nullptr.
        so_5::send_delayed(so_environment(), another_mbox, std::chrono::milliseconds(100),
                std::move(cmd));
    }
    
    void sixth_event_handler(mhood_t<sixth> cmd) {
        // Redirection of immutable message as service request.
        // After call to `request_future` cmd still contains valid pointer to the message.
        auto f = so_5::request_future<result>(another_mbox, cmd);
    }
    void seventh_event_handler(mhood_t<so_5::mutable_msg<seventh>> cmd) {
        // Redirection of mutable message as service request.
        // After call to `request_value` cmd becomes a nullptr.
        auto f = so_5::request_value<result>(another_mbox, so_5::infinite_wait, std::move(cmd));
    }
};
~~~~~
