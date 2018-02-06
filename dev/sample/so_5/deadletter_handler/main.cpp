#include <so_5/all.hpp>

// Demo agent which shows deadletter handler.
class demo final : public so_5::agent_t {
	// We need two signals. One will be handler by ordinary handler...
	struct first_signal final : public so_5::signal_t {};
	// ...and second by deadletter handler.
	struct second_signal final : public so_5::signal_t {};
public:
	demo(context_t ctx) : so_5::agent_t(std::move(ctx)) {
		// Create subscriptions.
		// The first signal will be handled as usual.
		so_subscribe_self().event([](mhood_t<first_signal>) {
				std::cout << "first_signal: ordinary handler" << std::endl;
			});
		// A deadletter handler will be used for the second.
		so_subscribe_deadletter_handler(
			so_direct_mbox(),
			[](mhood_t<second_signal>) {
				std::cout << "second_signal: deadletter handler" << std::endl;
			});
	}

	virtual void so_evt_start() override {
		// Send two different signal.
		so_5::send<first_signal>(*this);
		so_5::send<second_signal>(*this);

		// Agent is no more needed.
		so_deregister_agent_coop_normally();
	}
};

int main() {
	// Launch the SObjectizer with just one agent inside.
	so_5::launch([](so_5::environment_t & env) {
			env.register_agent_as_coop(so_5::autoname, env.make_agent<demo>());
		});
}

