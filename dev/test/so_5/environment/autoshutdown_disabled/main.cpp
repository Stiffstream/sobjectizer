/*
 * A simple test for checking disabling autoshutdown feature of SO Environment.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

std::atomic_uint agents_destroyed = ATOMIC_VAR_INIT( 0 );
std::atomic< so_5::environment_t * > environment =
	ATOMIC_VAR_INIT( nullptr );

const unsigned int agents_count = 1024;

struct msg_tick : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
		{}
		~a_test_t() override
		{
			++agents_destroyed;
		}

		void
		so_define_agent() override
		{
			so_subscribe_self().event( [this](mhood_t< msg_tick >) {
					m_ticks += 1;
					if( m_ticks < 15 )
						so_5::send< msg_tick >( *this );
					else
						so_deregister_agent_coop_normally();
				} );
		}

		void
		so_evt_start() override
		{
			so_5::send< msg_tick >( *this );
		}

	private :
		unsigned int m_ticks = 0;
};

void
controller()
{
	while( agents_count != agents_destroyed.load( std::memory_order_acquire ) )
		std::this_thread::yield();

	std::cout << "All agents are destroyed. Take SO Environment some time..."
			<< std::endl;

	std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

	auto env = environment.load( std::memory_order_acquire );
	if( !env )
	{
		std::cerr << "environment cannot be nullptr!" << std::endl;
		std::abort();
	}
	else
		std::cout << "Stopping SO Environment..." << std::endl;
	env->stop();
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				std::thread controller_thread( &controller );

				so_5::launch(
					[]( so_5::environment_t & env )
					{
						std::cout << "Starting agents..." << std::endl;

						environment.store( &env, std::memory_order_release );

						for( unsigned int i = 0; i < agents_count; ++i )
						{
							env.register_agent_as_coop(
								env.make_agent< a_test_t >() );
						}
					},
					[]( so_5::environment_params_t & params )
					{
						params.disable_autoshutdown();
					} );

				environment.store( nullptr, std::memory_order_release );

				controller_thread.join();
			},
			20,
			"SO Environment autoshutdown disabled test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

