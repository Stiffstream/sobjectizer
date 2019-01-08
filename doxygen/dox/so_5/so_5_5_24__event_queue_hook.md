# so-5.5.24 event_queue_hook {#so_5_5_24__event_queue_hook}

[TOC]

# What Is It?

SObjectizer-5.5.24 contains another feature for customization of SObjectizer Run-Time behaviour. It is an *event_queue_hook*.

Since v.5.5.24 there is a single instance of event_queue_hook in SObjectizer Environment. By default it is no-op hook (nothing is happened when this no-op hook is invoked). But user can set his/her own instance can in `environment_params_t` before launching of SObjectizer Environment.

Since v.5.5.24 every agent calls event_queue_hook two times:

* the first call to event_queue_hook is performed when agent is being bound to a particular event queue. Agent receives a pointer to event_queue provided by corresponding dispatcher and passes that pointer to `event_queue_hook_t::on_bind` method. As result agent receives a new pointer and this new pointer will be used by agent for enqueuing new demands;
* the second call to event_queue_hook is performed when agent is being unbound from the dispatcher. Agent passes a pointer to event_queue returned from `event_queue_hook_t::on_bind` to `event_queue_hook_t::on_unbind` method.

The event_queue_hook mechanism allows to make specialized wrappers around actual event queues. These wrappers can be used for different tasks. For example for tracing purposes or for gathering some run-time stats.

Please note that this is a low-level mechanism intended to be used for very specific tasks. Because its low-level nature the details and behavior of that mechanism can be changed dramatically in future versions of SObjectizer without any prior notice.

# A Simple Example

As a very simple example we provide a rather trivial implementation of event_queue_hook. This implementation wraps every event_queue in a special proxy object:
```{.cpp}
class demo_event_queue_hook final : public so_5::event_queue_hook_t {
	logger & sink_;

public:
	demo_event_queue_hook(logger & sink) : sink_{sink} {}

	SO_5_NODISCARD
	so_5::event_queue_t * on_bind(
			so_5::agent_t * /*agent*/,
			so_5::event_queue_t * original_queue ) SO_5_NOEXCEPT override {
		return new event_queue_logging_proxy{original_queue, sink_};
	}

	void on_unbind(
			so_5::agent_t * /*agent*/,
			so_5::event_queue_t * queue ) SO_5_NOEXCEPT override {
		delete queue;
	}
};
```
Where `event_queue_logging_proxy` can looks like:
```{.cpp}
class event_queue_logging_proxy final : public so_5::event_queue_t {
	so_5::event_queue_t * actual_queue_;
	logger & sink_;

public:
	event_queue_logging_proxy(
			so_5::event_queue_t * actual_queue,
			logger & sink)
		: actual_queue_{actual_queue}
		, sink_{sink}
	{
		sink_.log("logging_proxy=", this, " created");
	}
	~event_queue_logging_proxy() override {
		sink_.log("logging_proxy=", this, " destroyed");
	}

	void push(so_5::execution_demand_t demand) override {
		sink_.log("logging_proxy=", this,
				", receiver_ptr=", demand.m_receiver,
				", mbox_id=", demand.m_mbox_id,
				", msg_type=", demand.m_msg_type.name(),
				", limit_block=", demand.m_limit,
				", msg_ptr=", demand.m_message_ref.get());
		actual_queue_->push(std::move(demand));
	}
};
```
This event_queue_hook can be passed to SObjectizer Environment this way:
```{.cpp}
int main() {
	logger sink("_event_queue_trace.log");
	demo_event_queue_hook hook{sink};

	so_5::launch([](so_5::environment_t & env) {
			env.introduce_coop([](so_5::coop_t & coop) {
					coop.make_agent<parent_agent>();
				});
		},
		[&](so_5::environment_params_t & params) {

			params.event_queue_hook(
					so_5::event_queue_hook_unique_ptr_t{
							&hook,
							so_5::event_queue_hook_t::noop_deleter});

		});
}
```
In that case instance of `demo_event_queue_hook` is created on stack and because of that `noop_deleter` will be used with `event_queue_hook_unique_ptr_t`.

We can also create an instance of `demo_event_queue_hook` as dynamic object. In that case we can write:
```{.cpp}
int main() {
	logger sink("_event_queue_trace.log");

	so_5::launch([](so_5::environment_t & env) {
			env.introduce_coop([](so_5::coop_t & coop) {
					coop.make_agent<parent_agent>();
				});
		},
		[&](so_5::environment_params_t & params) {

			params.event_queue_hook(
					so_5::make_event_queue_hook<demo_event_queue_hook>(
							so_5::event_queue_hook_t::default_deleter,
							sink));
		});
}
```
