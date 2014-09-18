/*
 * A sample of the simpliest agent.
 */

#include <iostream>

// Main SObjectizer headers.
#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>
#include <so_5/rt/h/coop_listener.hpp>

// An agent class definition.
class a_hello_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:
		a_hello_t( so_5::rt::so_environment_t & env )
			: base_type_t( env )
		{}

		virtual ~a_hello_t()
		{}
};

// The SObjectizer Environment initialization.
void
init( so_5::rt::so_environment_t & env )
{
	// Creating and registering a cooperation.
	env.register_agent_as_coop( "coop", new a_hello_t( env ) );

	// Stopping SObjectizer.
	env.stop();
}

// A class for listening cooperation events.
class coop_listener_impl_t
	:
		public so_5::rt::coop_listener_t
{
	public:
		virtual ~coop_listener_impl_t()
		{}

		// A reaction to the cooperation registration.
		virtual void
		on_registered(
			so_5::rt::so_environment_t & so_env,
			const std::string & coop_name ) throw()
		{
			std::cout << "coop_listener: register coop '"
				<< coop_name << "'\n";
		}

		// A reaction to the cooperation deregistration.
		virtual void
		on_deregistered(
			so_5::rt::so_environment_t & so_env,
			const std::string & coop_name,
			const so_5::rt::coop_dereg_reason_t & reason ) throw()
		{
			std::cout << "coop_listener: deregister coop '"
				<< coop_name << "', reason: "
				<< reason.reason() << "\n";
		}
};

int
main( int, char ** )
{
	try
	{
		so_5::api::run_so_environment(
			&init,
			[]( so_5::rt::so_environment_params_t & p ) {
				// Adding a cooperation listener to show what happened
				// with the sample cooperation.
				p.coop_listener(
					so_5::rt::coop_listener_unique_ptr_t(
						new coop_listener_impl_t ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
