/*
 * A test for checking autoshutdown during execution of init function.
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

class a_first_t : public so_5::agent_t
{
	public :
		a_first_t( so_5::environment_t & env )
			:	so_5::agent_t( env )
		{}

		virtual void
		so_evt_start() override
		{
			so_deregister_agent_coop_normally();
		}
};

class a_second_t : public so_5::agent_t
{
		struct msg_timer : public so_5::signal_t {};

	public :
		a_second_t(
			so_5::environment_t & env,
			int * receiver )
			:	so_5::agent_t( env )
			,	m_receiver( *receiver )
		{}

		virtual void
		so_define_agent() override
		{
			so_subscribe_self().event( [this](mhood_t< msg_timer >) {
					m_receiver = 42;
					so_deregister_agent_coop_normally();
				} );
		}

		virtual void
		so_evt_start() override
		{
			so_5::send_delayed< msg_timer >( *this, std::chrono::milliseconds(250) );
		}

	private :
		int & m_receiver;
};

int
main()
{
	try
	{
		int actual = 0;

		run_with_time_limit(
			[&actual]()
			{
				so_5::launch(
					[&actual]( so_5::environment_t & env )
					{
						env.register_agent_as_coop(
								env.make_agent< a_first_t >() );

						// Time for the first cooperation deregistration.
						std::this_thread::sleep_for(
							std::chrono::milliseconds( 250 ) );

						env.register_agent_as_coop(
							env.make_agent< a_second_t >( &actual ) );
					} );
			},
			20,
			"SO Environment autoshutdown_in_run test" );

		if( 42 != actual )
			throw std::runtime_error( "42 != actual!" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

