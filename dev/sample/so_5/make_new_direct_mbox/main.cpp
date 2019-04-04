/*
 * A sample of usage of so_make_new_direct_mbox() method.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Definition of an agent for SObjectizer.
class a_demo_t final : public so_5::agent_t
{
		// Signals to be used by this agent.
		struct msg_hello final : public so_5::signal_t {};
		struct msg_bye final : public so_5::signal_t {};

	public:
		using so_5::agent_t::agent_t;

		void so_evt_start() override
		{
			// Make subscriptions for the default direct mbox.
			so_subscribe_self()
				.event( [](mhood_t<msg_hello>) {
						std::cout << "hello from the default mbox" << std::endl;
					} )
				.event( [this](mhood_t<msg_bye>) {
						// Work should be finished.
						so_deregister_agent_coop_normally();
					} );

			// Create another direct mbox and make subscription for it.
			const auto another = so_make_new_direct_mbox();
			so_subscribe( another )
				.event( [](mhood_t<msg_hello>) {
						std::cout << "hello from a new direct mbox" << std::endl;
					} );

			// Send signals.
			so_5::send< msg_hello >( *this );
			so_5::send< msg_hello >( another );

			so_5::send< msg_bye >( *this );
		}
};

int main()
{
	try
	{
		// Starting SObjectizer.
		so_5::launch(
			// A function for SO Environment initialization.
			[]( so_5::environment_t & env )
			{
				// Creating and registering single agent as a cooperation.
				env.register_agent_as_coop( env.make_agent< a_demo_t >() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
