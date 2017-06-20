# so-5.5.19 Custom mbox Creation Procedure {#so_5_5_19__custom_mbox}

Sometimes it is necessary to create and use a custom implementation of `so_5::abstract_message_box_t`. For example, so-5-extra project contains several custom implementations of mboxes inside. But there was a problem with custom mboxes: every mbox must have an unique ID (this ID is a long integer represented by `so_5::mbox_id_t` type). These IDs are distributed by SObjectizer Environment. If someone will try to use its own values for mbox ID there can be a conflict between IDs from SObjectizer Environment and someone's IDs.

Another problem is related to message tracing mechanism. Mboxes must take into account message tracing stuff and must implement message delivery tracing if such tracing is used. It means that custom mbox should receive a pointer to optional `so_5::msg_tracing::tracer_t` object.

Since v.5.5.19.2 there is a unified custom mbox creation procedure. It is a template method `so_5::environment_t::make_custom_mbox()`. This template method receives a lambda-function or a functional object which is called inside `make_custom_mbox()`. A single agrument of type `so_5::mbox_creation_data_t` is passed to that lambda-function. That argument contains mbox ID generated for the new mbox by SObjectizer Environment. It also contains an optional pointer to message tracer. The lambda-function should return a new message box (in for of `so_5::mbox_t`).

For example:
~~~~~{.cpp}
// A custom implementation of message box.
class my_special_mbox : public so_5::abstract_message_box_t
{
public:
    // Constructor receives several important arguments...
    my_special_mbox(
        // Unique ID for that mbox.
        so_5::mbox_id_t id,
        // An optional message tracer.
        so_5::msg_tracing::tracer_t * tracer)
    {...}
    ...
};

// Create an instance of my_special_mbox.
auto mbox = so_environment().make_custom_mbox(
    [](const so_5::mbox_creation_data_t & data) {
        return so_5::mbox_t(new my_special_mbox(data.m_id, data.m_tracer));
    });
~~~~~
