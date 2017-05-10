/*
 * An example of receiving message of type M as immutable and as mutable ones.
 */

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Definition of an agent for SObjectizer.
class two_handlers final : public so_5::agent_t {
	// Message to be sent as mutable and immutable.
	struct M final {};
	// Helper signal to stop the example.
	struct stop final : public so_5::signal_t {};

public :
	two_handlers(context_t ctx) : so_5::agent_t(std::move(ctx)) {
		so_subscribe_self()
			.event(&two_handlers::on_immutable_M)
			.event(&two_handlers::on_mutable_M)
			.event(&two_handlers::on_stop);
	}

	virtual void so_evt_start() override {
		so_5::send<M>(*this); // Immutable message is sent.
		so_5::send<so_5::mutable_msg<M>>(*this); // Mutable message is sent.

		so_5::send<stop>(*this); // Finish the work.
	}
private :
	void on_immutable_M(mhood_t<M>) {
		std::cout << "on immutable" << std::endl;
	}
	void on_mutable_M(mhood_t<so_5::mutable_msg<M>>) {
		std::cout << "on mutable" << std::endl;
	}

	void on_stop(mhood_t<stop>) { so_deregister_agent_coop_normally(); }
};

int main()
{
	// Starting SObjectizer.
	so_5::launch(
		// A function for SO Environment initialization.
		[](so_5::environment_t & env)
		{
			// Creating and registering single agent as a cooperation.
			env.register_agent_as_coop(so_5::autoname, env.make_agent<two_handlers>() );
		} );

	return 0;
}
