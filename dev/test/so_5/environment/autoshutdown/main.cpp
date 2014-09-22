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

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_tick : public so_5::rt::signal_t {};

class a_test_t : public so_5::rt::agent_t
{
	public:
		a_test_t(
			so_5::rt::so_environment_t & env )
			:	so_5::rt::agent_t( env )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe( so_direct_mbox() ).event(
					so_5::signal< msg_tick >,
					[this]() {
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
main( int argc, char * argv[] )
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::api::run_so_environment(
					[]( so_5::rt::so_environment_t & env )
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
			5,
			"SO Environment autoshutdown test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

