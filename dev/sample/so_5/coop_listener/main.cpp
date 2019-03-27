/*
 * A sample of the simpliest agent.
 */

#include <iostream>

// Main SObjectizer header.
#include <so_5/all.hpp>

// An agent class definition.
class a_hello_t : public so_5::agent_t
{
	public:
		a_hello_t( context_t ctx ) : so_5::agent_t( ctx )
		{}
};

// The SObjectizer Environment initialization.
void init( so_5::environment_t & env )
{
	// Creating and registering a cooperation.
	env.register_agent_as_coop( env.make_agent< a_hello_t >() );

	// Stopping SObjectizer.
	env.stop();
}

// A class for listening cooperation events.
class coop_listener_impl_t : public so_5::coop_listener_t
{
	public:
		virtual ~coop_listener_impl_t() override
		{}

		// A reaction to the cooperation registration.
		void on_registered(
			so_5::environment_t &,
			const so_5::coop_handle_t & coop ) noexcept override
		{
			std::cout << "coop_listener: register coop '"
				<< coop << "'\n";
		}

		// A reaction to the cooperation deregistration.
		void on_deregistered(
			so_5::environment_t &,
			const so_5::coop_handle_t & coop,
			const so_5::coop_dereg_reason_t & reason ) noexcept override
		{
			std::cout << "coop_listener: deregister coop '"
				<< coop << "', reason: "
				<< reason.reason() << "\n";
		}
};

int main()
{
	try
	{
		so_5::launch(
			&init,
			[]( so_5::environment_params_t & p ) {
				// Adding a cooperation listener to show what happened
				// with the sample cooperation.
				p.coop_listener( std::make_unique< coop_listener_impl_t >() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
