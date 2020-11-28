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

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

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

		~a_ordinary_t() override
		{
			--g_agents_count;
		}

		void
		so_define_agent() override
		{
			so_5::mbox_t mbox = so_environment()
				.create_mbox( g_test_mbox_name );

			so_subscribe( mbox )
				.in( so_default_state() )
					.event( &a_ordinary_t::some_handler );

			// Give time to test message sender.
			std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		}

		void
		so_evt_start() override;

		void
		some_handler( mhood_t< some_message > );
};

void
a_ordinary_t::so_evt_start()
{
	++g_evt_count;
}

void
a_ordinary_t::some_handler( mhood_t< some_message > )
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

		~a_throwing_t() override
		{
			--g_agents_count;
		}

		void
		so_evt_start() override;
};

class throwing_disp_binder_t
	:
		public so_5::disp_binder_t
{
	public:
		void
		preallocate_resources(
			so_5::agent_t & /*agent*/ ) override
		{
			throw std::runtime_error(
				"throwing while binding agent to disp" );
		}

		void
		undo_preallocation(
			so_5::agent_t & /*agent*/ ) noexcept override {}

		void
		bind(
			so_5::agent_t & /*agent*/ ) noexcept override {}

		virtual void
		unbind(
			so_5::agent_t & /*agent*/ ) noexcept override {}
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
	auto coop = env.make_coop();

	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();

	// This agent will throw an exception during binding for dispatcher.
	coop->make_agent_with_binder< a_throwing_t >(
			std::make_shared< throwing_disp_binder_t >() );

	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();

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
	run_with_time_limit( [] {
			so_5::launch( &init );

			if( 0 != g_agents_count )
			{
				std::cerr << "g_agents_count: " << g_agents_count << "\n";
				throw std::runtime_error( "g_agents_count != 0" );
			}

			std::cout << "event handled: " << g_evt_count << "\n";
		},
		10 );

	return 0;
}
