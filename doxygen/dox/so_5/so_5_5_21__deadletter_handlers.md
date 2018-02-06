# so-5.5.21 Deadletter Handlers {#so_5_5_21__deadletter_handlers}

[TOC]

# The Problem
There is a SObjectizer's key feature: states of an agent. An agent can have several states. It allows to handle the same message in every state differently. For example:

~~~~~{.cpp}
class demo : public so_5::agent_t {
	state_t st_first{this}, st_second{this}, st_third{this};
	...
	virtual void so_define_agent() override {
		st_first.event([this](mhood_t<msg>) { std::cout << "1!" << std::endl; });
		st_second.event([this](mhood_t<msg>) { std::cout << "2!" << std::endl; });
		st_third.event([this](mhood_t<msg>) { std::cout << "3!" << std::endl; });
		...
	}
};
~~~~~

It you send a `msg` to an agent of type `demo` then this agent will print different messages in the dependence of the current agent's state.

This feature is very useful and it is used extensively in the complex applications. But there is the dark side: if an agent has several states and has to handle some message regardless of the current state then it requires much more work from a developer.

There were two ways for implementation of message handling in every state of an agent:

The first one requires subscription of the same event for every state of the agent. Something like:

~~~~~{.cpp}
class demo : public so_5::agent_t {
	state_t st_first{this}, st_second{this}, st_third{this};
	...
	void on_some_message(mhood_t<some_msg> cmd) {...}

	virtual void so_define_agent() override {
		st_first.event(&demo::on_some_message);
		st_second.event(&demo::on_some_message);
		st_third.event(&demo::on_some_message);
		...
	}
};
~~~~~
Or:
~~~~~{.cpp}
virtual void so_define_agent() override {
	so_subscribe_self()
		.in(st_first)
		.in(st_second)
		.in(st_third)
		.event(&demo::on_some_message);
}
~~~~~
But this approach is boring and it is easy to forget to subscribe the event handler in a new state when this state will be added to agent.

The second approach is more flexible and powerful. It is based on hierarchical state machines. There should be one parent state and several child states. An event handler is subscribed only for the parent state:

~~~~~{.cpp}
class demo : public so_5::agent_t {
	state_t st_parent{this};
	state_t st_first{ initial_substate_of{st_parent} },
		st_second{ substate_of{st_parent} },
		st_third{ substate_of{st_parent} };

	void on_some_message(mhood_t<some_msg> cmd) {...}

	virtual void so_define_agent() override {
		st_parent.event(&demo::on_some_message);
		...
	}
};
~~~~~

This approach will work if an user add or remove child states to agent of type `demo`. It also allows to have different event handlers for the same message in different states:

~~~~~{.cpp}
class demo : public so_5::agent_t {
	state_t st_parent{this};
	state_t st_first{ initial_substate_of{st_parent} },
		st_second{ substate_of{st_parent} },
		st_third{ substate_of{st_parent} };

	void on_some_message_default(mhood_t<some_msg> cmd) {...}

	virtual void so_define_agent() override {
		st_parent.event(&demo::on_some_message_default);
		st_second.event([this](mhood_t<some_msg> cmd) {
				... /* Different code. */
			});
		...
	}
};
~~~~~

But this approach is not always applicable. For example you can have a base agent class which we can't modify and you have to write a derived class with a default event handler for the some message:

~~~~~{.cpp}
// Some base class. Maybe it is defined in third-party library.
class base : public so_5::agent_t {
	state_t st_first{...}, ... /* some other states */ ...;
	...
};

// Derived class. Adds some its own states. And needs to install a default event-handler.
class derived : public base {
	state_t st_another_state{...}, st_yet_another_state{...}, ...;
	...
	void some_default_handler(mhood_t<some_msg> cmd) {...}
	...
};
~~~~~

It is possible that the base class doesn't use HSM and don't have the top-level parent state. Or all states of base class can be declared as private attributes and you have no access to it in your derived class.
# Deadletter Handlers as a Solution
A new thing was introduced in SObjectizer v.5.5.21: a deadletter handler.

Deadletter handler is a like an ordinary message handler. But it is subscribed and unsubscribed by different methods. And the key moment: deadletter is called only when there is no any other event handlers for a message from a specific mbox.

For example:
~~~~~{.cpp}
class demo : public so_5::agent_t {
	struct first_signal final : public so_5::signal_t {};
	struct second_signal final : public so_5::signal_t {};
	...
	virtual void so_define_agent() override {
		so_subscribe_self().event([](mhood_t<first_signal>) {
				std::cout << "first_signal: ordinary handler" << std::endl;
			});
		so_subscribe_deadletter_handler(
			so_direct_mbox(),
			[](mhood_t<second_signal>) {
				std::cout << "second_signal: deadletter handler" << std::endl;
			});
	}
	virtual void so_evt_start() override {
		so_5::send<first_signal>(*this);
		so_5::send<second_signal>(*this);
	}
};
~~~~~
This example will print:

~~~~~
first_signal: ordinary handler
second_signal: deadletter handler
~~~~~

It is because there is no more event handlers for `second_signal` from agent's direct mbox except the deadletter handler. But if we modify the example such way:

~~~~~{.cpp}
class demo : public so_5::agent_t {
	struct first_signal final : public so_5::signal_t {};
	struct second_signal final : public so_5::signal_t {};
	...
	virtual void so_define_agent() override {
		so_subscribe_self().event([](mhood_t<first_signal>) {
				std::cout << "first_signal: ordinary handler" << std::endl;
			});
		so_subscribe_self().event([](mhood_t<second_signal>) {
				std::cout << "second_signal: ordinary handler" << std::endl;
			});
		so_subscribe_deadletter_handler(
			so_direct_mbox(),
			[](mhood_t<second_signal>) {
				std::cout << "second_signal: deadletter handler" << std::endl;
			});
	}
	virtual void so_evt_start() override {
		so_5::send<first_signal>(*this);
		so_5::send<second_signal>(*this);
	}
};
~~~~~
The output will be:
~~~~~
first_signal: ordinary handler
second_signal: ordinary handler
~~~~~
It is because the SObjectizer will find an ordinary handler for `second_signal` from agent's direct mbox and will call it. The deadletter handler for `second_signal` won't be called because there is an ordinary handler.

In other worlds since v.5.5.21 SObjectizer implements a modified algorithm for searching event handlers for a message/signal. SObjectizer does old-way search for a message handler first with respect to the current agent's state. If an event handler is found it will be called and message processing will be finished. But if an event handler for the current state (and all of its parent states) won't be found then SObjectizer will search for deadletter handler for that message and that mbox. It a deadletter handler will be found it will be called for handling of message.
# An API for Dealing with Deadletter Handlers
An API for working with deadletter handlers was introduced in v.5.5.21. Class `so_5::agent_t` was extended with the following methods.
## so_subscribe_deadletter_handler
This method is intended to be used for subscription of deadletter handler. For example:

~~~~~{.cpp}
virtual void my_agent::so_define_agent() override {
	// Create a deadleatter handler for messages from the direct mbox.
	// Deadletter handler will be subscribed as thread-unsafe handler.
	so_subscribe_deadletter_handler(so_direct_mbox(), &my_agent::some_handler);

	// Lambda-function can be used as event-handler too.
	so_subscribe_deadletter_handler(some_mbox,
		[](mhood_t<message_type> cmd) {...});

	// Thread-safety flag for event-handler can be specified too.
	so_subscribe_deadletter_handler(some_mbox,
		&my_agent::some_thread_safe_handler,
		so_5::thread_safe);
}
~~~~~

A event handler which will be passed to `so_subscribe_deadletter_handler` must be a pointer to method of agent's class or lambda-function with one of the following formats:

~~~~~{.cpp}
ret_type handler(message_type);
ret_type handler(message_type) const;
ret_type handler(const message_type &);
ret_type handler(const message_type &) const;
ret_type handler(mhood_t<message_type>);
ret_type handler(mhood_t<message_type>) const;
ret_type handler(const mhood_t<message_type> &);
ret_type handler(const mhood_t<message_type> &) const;
~~~~~

It means that if a deadletter handler has to handle signals then it should has the following format:
~~~~~{.cpp}
ret_type handler(mhood_t<signal_type>);
~~~~~
For example:
~~~~~{.cpp}
class demo : public so_5::agent_t {
	struct demo_sig final : public so_5::signal_t {};
	...
	virtual void so_define_agent() override {
		...
		so_subscribe_deadletter_handler(
			so_direct_mbox(), [this](mhood_t<demo_sig>) {...});
		}
};
~~~~~

A message type for a deadletter handler can also be a mutable message. For example:
~~~~~{.cpp}
class demo : public so_5::agent_t {
	void on_some_mutable_msg(mutable_mhood_t<some_msg> cmd) {...}
	...
	virtual void so_define_agent() override {
		...
		so_subscribe_deadletter_handler(
			so_direct_mbox(), &demo::on_some_mutable_msg);
		...
	}
	virtual void so_evt_start() override {
		...
		so_5::send< so_5::mutable_msg<some_msg> >(*this, ...);
	}
};
~~~~~

**NOTE.** `so_subscribe_deadletter_handler` throws if a deadletter handler for a specific message type from a specific mbox is already defined.
## so_drop_deadletter_handler
This method should be used for unsubscription of deadletter handler. For example:
~~~~~{.cpp}
struct completion_signal final : public so_5::signal_t {};
class demo : public so_5::agent_t {
	so_5::mbox_t completion_mbox_;

	void actual_completion_handler(mhood_t<completion_signal>) {
		... /* Do some stuff. */
		// Default completion handler is not needed and can be removed.
		so_drop_deadletter_handler<completion_signal>(completion_mbox_);
	}
	void default_completion_handler(mhood_t<completion_signal>) {
		... /* Do some default stuff. */
		// Default completion handler is not needed and can be removed.
		so_drop_deadletter_handler<completion_signal>(completion_mbox_);
	}

	void on_some_request(mhood_t<request_data> cmd) {
		// Store mbox from which a completion signal is expected.
		completion_mbox_ = cmd->operation_mbox();
		// Create subscription for completion signal.
		// An actual handler for the state in which we expect the completion signal.
		so_subscribe(completion_mbox_)
			.in(one_selected_state)
			.event(&demo::actual_completion_handler);
		// For all other states a completion signal will be handled by a deadletter handler.
		so_subscribe_deadletter_handler(completion_mbox_,
			&demo::default_completion_handler);
		...
}
};
~~~~~

It is safe to call `so_drop_deadletter_handler` for deadletter handler which is already removed. There won't be an exception.

**NOTE.** A deadletter handler will also be removed by `so_drop_subscription_for_all_states` (see the corresponding section below).
## so_has_deadletter_handler
This method is intended to be used for checking the presence of a deadletter handler for a specific type of message from a specific mbox. For example:

~~~~~{.cpp}
void my_agent::on_request(mhood_t<request_data> cmd) {
	if(!so_has_deadletter_handler<completion_signal>(cmd->operation_mbox())) {
		// There is no default completion handler yet.
		// It should be created.
		so_subscribe_deadletter_handler(
			cmd->operation_mbox(),
			&my_agent::default_completion_handler);
	}
	...
}
~~~~~
## Other Methods
There are also `so_create_deadletter_subscription` and `so_destroy_deadletter_subscription`. But they are low-level methods intended to be used by libraries writers. Do not use it directly if you don't understand what they are doing and which arguments they require.
# Some Additional Information About Deadletter Handlers
## Deadletter Handlers and Service Requests
Deadletter handlers are just ordinary message handlers. Because of that they can handle service requests. For example:
~~~~~{.cpp}
class my_device_handler : public some_3rd_party::lib::basic_device {
	state_t st_working{this};
public:
	struct current_status final : public so_5::signal_t {};
	...
	virtual void so_define_agent() override {
		// Allow base class to initialize itself.
		some_3rd_party::lib::basic_device::so_define_agent();

		// Make own subscriptions.
		// Ordinary handler for our own state.
		st_working.event(
			[](mhood_t<current_status>) -> std::string {
				return "working";
			});
		// And deadletter handler for all other states from base type.
		so_subscribe_deadletter_handler(
			so_direct_mbox(),
			[](mhood_t<current_status>) -> std::string {
				return "unknown";
			},
			so_5::thread_safe);
		...
	}
	...
};
...
// Somewhere is the program.
auto status = so_5::request_value<std::string, my_device_handler::current_status>(
	my_device_mbox, so_5::infinite_wait);
~~~~~
## Deadletter Handlers are Removed by so_drop_subscription_for_all_states
A deadletter handler can be removed (unsubscribed) not only by `so_drop_deadletter_handler` but also by `so_drop_subscription_for_all_states`. It means that in that case:
~~~~~{.cpp}
class drop_all_demo : public so_5::agent_t {
	state_t st_working{this}, st_waiting{this}, st_busy{this};
	void make_handlers() {
		so_subscribe(some_mbox)
			.in(st_working).in(st_busy)
			.event(&drop_all_demo::on_some_message);
		so_subscribe_deadletter_handler(
			some_mbox,
			&drop_all_demo::default_handler);
	}
	...
	void on_some_message(mhood_t<some_message> cmd) {
		... // Do some stuff.
		// Drop all subscriptions for some_message.
		so_drop_subscription_for_all_states<some_message>(some_mbox);
		// Now there is no any subscription to some_message from some_mbox.
		// Event deadletter for that message was removed.
	}
};
~~~~~
