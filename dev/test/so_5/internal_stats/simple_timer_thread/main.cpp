/*
 * A simple test for getting count of timers from run-time monitoring messages.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
	{
	public :
		struct msg_delayed : so_5::signal_t {};

		a_test_t( context_t ctx )
			:	so_5::agent_t( ctx )
			{}

		virtual void
		so_define_agent() override
			{
				so_default_state().event(
						so_environment().stats_controller().mbox(),
						&a_test_t::evt_monitor_quantity );
			}

		virtual void
		so_evt_start() override
			{
				using namespace std::chrono;

				so_5::send_delayed< msg_delayed >( *this, seconds( 10 ) );
				so_5::send_delayed< msg_delayed >( *this, seconds( 10 ) );

				m_p1 = so_5::send_periodic< msg_delayed >( *this,
						seconds( 10 ), seconds( 10 ) );
				m_p2 = so_5::send_periodic< msg_delayed >( *this,
						seconds( 10 ), seconds( 10 ) );
				m_p3 = so_5::send_periodic< msg_delayed >( *this,
						seconds( 10 ), seconds( 10 ) );

				so_environment().stats_controller().turn_on();
			}

	private :
		unsigned int m_actual_values = { 0 };

		so_5::timer_id_t m_p1;
		so_5::timer_id_t m_p2;
		so_5::timer_id_t m_p3;

		void
		evt_monitor_quantity(
			const so_5::stats::messages::quantity< std::size_t > & evt )
			{
				namespace stats = so_5::stats;

				std::cout << evt.m_prefix.c_str()
						<< evt.m_suffix.c_str()
						<< ": " << evt.m_value << std::endl;

				if( stats::prefixes::timer_thread() == evt.m_prefix )
					{
						if( stats::suffixes::timer_single_shot_count() == evt.m_suffix )
							{
								if( 2 != evt.m_value )
									throw std::runtime_error( "unexpected count of "
											"single-shot timers: " +
											std::to_string( evt.m_value ) );
								else
									++m_actual_values;
							}
						else if( stats::suffixes::timer_periodic_count() == evt.m_suffix )
							{
								if( 3 != evt.m_value )
									throw std::runtime_error( "unexpected count of "
											"periodic timers: " +
											std::to_string( evt.m_value ) );
								else
									++m_actual_values;
							}
					}

				if( 2 == m_actual_values )
					so_deregister_agent_coop_normally();
			}
	};

void
init( so_5::environment_t & env )
	{
		env.register_agent_as_coop( so_5::autoname,
				env.make_agent< a_test_t >() );
	}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			20,
			"simple timer thread monitoring test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

