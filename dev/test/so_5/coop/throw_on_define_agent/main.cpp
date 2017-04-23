/*
 * A test for handling of exception during so_define_agent() calling.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>

#include <so_5/all.hpp>

const char * const g_test_mbox_name = "test_mbox";

struct some_message : public so_5::signal_t {};

class a_ordinary_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:

		a_ordinary_t( so_5::environment_t & env )
			:
				base_type_t( env )
		{}

		virtual ~a_ordinary_t()
		{}

		virtual void
		so_define_agent()
		{
			so_5::mbox_t mbox = so_environment()
				.create_mbox( g_test_mbox_name );

			so_subscribe( mbox )
				.in( so_default_state() )
					.event( &a_ordinary_t::some_handler );

			// Give some time to agent which sends messages.
			std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		}

		virtual void
		so_evt_start();

		void
		some_handler(
			const so_5::event_data_t< some_message > & );
};

void
a_ordinary_t::so_evt_start()
{
	// This method should not be called.
	std::cerr << "error: a_ordinary_t::so_evt_start called.";
	std::abort();
}



void
a_ordinary_t::some_handler(
	const so_5::event_data_t< some_message > & )
{
	// This method should not be called.
	std::cerr << "error: a_ordinary_t::some_handler called.";
	std::abort();
}

class a_throwing_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:
		a_throwing_t( so_5::environment_t & env )
			:
				base_type_t( env )
		{}

		virtual ~a_throwing_t()
		{}

		virtual void
		so_define_agent()
		{
			throw std::runtime_error(
				"test throwing while defining agent" );
		}

		virtual void
		so_evt_start();
};

void
a_throwing_t::so_evt_start()
{
	// This method should not be called.
	std::cerr << "error: a_throwing_t::so_evt_start called.";
	std::abort();
}

void
reg_coop(
	so_5::environment_t & env )
{
	so_5::coop_unique_ptr_t coop =
		env.create_coop( "test_coop" );

	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );

	// An agent which will throw an exception.
	coop->add_agent( new a_throwing_t( env ) );

	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );

	try {
		env.register_coop( std::move( coop ) );
	}
	catch(...) {}
}

void
init( so_5::environment_t & env )
{
	reg_coop( env );
	env.stop();
}

int
main()
{
	try
	{
		so_5::launch(
			&init,
			[]( so_5::environment_params_t & params )
			{
				params.add_named_dispatcher(
					"active_obj",
					so_5::disp::active_obj::create_disp() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
