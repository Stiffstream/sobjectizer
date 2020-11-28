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
			so_subscribe( so_direct_mbox() )
				.in( so_default_state() )
					.event( &a_ordinary_t::some_handler );

			so_5::send< some_message >( so_direct_mbox() );
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

class throwing_disp_binder_t
	:
		public so_5::disp_binder_t
{
	public:
		void
		preallocate_resources(
			so_5::agent_t & /*agent*/ ) override
		{
			std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

			throw std::runtime_error(
				"throwing while binding agent to disp" );
		}

		void
		undo_preallocation(
			so_5::agent_t & /*agent*/ ) noexcept override {}

		void
		bind(
			so_5::agent_t & /*agent*/ ) noexcept override {}

		void
		unbind(
			so_5::agent_t & /*agent*/ ) noexcept override {}
};

void
reg_coop(
	so_5::environment_t & env )
{
	auto coop = env.make_coop(
			so_5::disp::active_obj::make_dispatcher( env ).binder() );

	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();

	// This agent will throw an exception during binding for dispatcher.
	coop->make_agent_with_binder< a_ordinary_t >(
			std::make_shared< throwing_disp_binder_t >() );

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
	run_with_time_limit( [] {
			so_5::launch( &init );

			if( 0 != g_agents_count )
			{
				std::cerr << "g_agents_count: " << g_agents_count << "\n";
				throw std::runtime_error( "g_agents_count != 0" );
			}
		},
		10 );

	return 0;
}

