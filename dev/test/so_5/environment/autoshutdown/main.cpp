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

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_tick : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe_self().event< msg_tick >( [=] {
					m_ticks += 1;
					if( m_ticks < 15 )
						so_direct_mbox()->deliver_signal< msg_tick >();
					else
						so_deregister_agent_coop_normally();
				} );
		}

		virtual void
		so_evt_start()
		{
			so_direct_mbox()->deliver_signal< msg_tick >();
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
							std::ostringstream ss;
							ss << "coop_" << i;

							env.register_agent_as_coop(
								ss.str(), new a_test_t( env ) );
						}
					} );
			},
			20,
			"SO Environment autoshutdown test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

