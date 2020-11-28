/*
 * A test for handling of exception during so_define_agent() calling.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

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

		void
		so_define_agent() override
		{
			so_5::mbox_t mbox = so_environment()
				.create_mbox( g_test_mbox_name );

			so_subscribe( mbox )
				.in( so_default_state() )
					.event( &a_ordinary_t::some_handler );

			// Give some time to agent which sends messages.
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
	// This method should not be called.
	std::cerr << "error: a_ordinary_t::so_evt_start called.";
	std::abort();
}

void
a_ordinary_t::some_handler( mhood_t< some_message > )
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

		void
		so_define_agent() override
		{
			throw std::runtime_error(
				"test throwing while defining agent" );
		}

		void
		so_evt_start() override;
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

	// An agent which will throw an exception.
	coop->make_agent< a_throwing_t >();

	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();

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
	run_with_time_limit( [] {
			so_5::launch( &init );
		},
		10 );

	return 0;
}
