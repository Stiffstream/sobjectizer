/*
 * A simple test for checking autoshutdown feature of SO Environment.
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

struct msg_tick : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
		{}

		void
		so_define_agent() override
		{
			so_subscribe_self().event( [=](mhood_t< msg_tick >) {
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

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch(
					[]( so_5::environment_t & env )
					{
						for( int i = 0; i < 1024; ++i )
						{
							env.register_agent_as_coop(
								env.make_agent< a_test_t >() );
						}
					} );
			},
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

