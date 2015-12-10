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
			so_subscribe( so_direct_mbox() )
				.in( so_default_state() )
					.event( &a_ordinary_t::some_handler );

			so_direct_mbox()->deliver_signal< some_message >();
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
			std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

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
reg_coop(
	so_5::environment_t & env )
{
	so_5::coop_unique_ptr_t coop = env.create_coop( "test_coop",
			so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );

	// This agent will throw an exception during binding for dispatcher.
	coop->add_agent(
		new a_ordinary_t( env ),
		so_5::disp_binder_unique_ptr_t( new throwing_disp_binder_t ) );

	try
	{
		env.register_coop( std::move( coop ) );
	}
	catch( const std::exception & x )
	{
		std::cout << "throw_on_bind_to_disp_2, expected exception: "
			<< x.what() << std::endl;
	}
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
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

