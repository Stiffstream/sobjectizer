/*
 * A test of handling an exception during binding to dispatcher.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

so_5::atomic_counter_t g_agents_count;
so_5::atomic_counter_t g_evt_count;

const char * const g_test_mbox_name = "test_mbox";

struct some_message : public so_5::signal_t {};

class a_ordinary_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:
		a_ordinary_t(
			so_5::environment_t & env )
			:
				base_type_t( env )
		{
			++g_agents_count;
		}

		virtual ~a_ordinary_t()
		{
			--g_agents_count;
		}

		virtual void
		so_define_agent()
		{
			so_5::mbox_t mbox = so_environment()
				.create_mbox( g_test_mbox_name );

			so_subscribe( mbox )
				.in( so_default_state() )
					.event( &a_ordinary_t::some_handler );

			// Give time to test message sender.
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
	++g_evt_count;
}

void
a_ordinary_t::some_handler(
	const so_5::event_data_t< some_message > & )
{
	++g_evt_count;
}

class a_throwing_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:

		a_throwing_t(
			so_5::environment_t & env )
			:
				base_type_t( env )
		{
			++g_agents_count;
		}

		virtual ~a_throwing_t()
		{
			--g_agents_count;
		}

		virtual void
		so_define_agent()
		{
		}

		virtual void
		so_evt_start();
};

class throwing_disp_binder_t
	:
		public so_5::disp_binder_t
{
	public:
		throwing_disp_binder_t() {}
		virtual ~throwing_disp_binder_t() {}

		virtual so_5::disp_binding_activator_t
		bind_agent(
			so_5::environment_t &,
			so_5::agent_ref_t )
		{
			throw std::runtime_error(
				"throwing while binding agent to disp" );
		}

		virtual void
		unbind_agent(
			so_5::environment_t &,
			so_5::agent_ref_t )
		{
		}

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

	// This agent will throw an exception during binding for dispatcher.
	coop->add_agent(
		new a_throwing_t( env ),
		so_5::disp_binder_unique_ptr_t( new throwing_disp_binder_t ) );

	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );

	try
	{
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

		if( 0 != g_agents_count )
		{
			std::cerr << "g_agents_count: " << g_agents_count << "\n";
			throw std::runtime_error( "g_agents_count != 0" );
		}

		std::cout << "event handled: " << g_evt_count << "\n";
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
