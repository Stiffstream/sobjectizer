/*
 * A simple test for message limits (dropping the message).
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

struct msg_one : public so_5::rt::signal_t {};

class a_test_t : public so_5::rt::agent_t
{
public :
	a_test_t(
		so_5::rt::environment_t & env )
		:	so_5::rt::agent_t( env + limit_then_abort< msg_one >( 1 ) )
	{}

	void
	set_working_mbox( const so_5::rt::mbox_t & mbox )
	{
		m_working_mbox = mbox;
	}

	virtual void
	so_define_agent() override
	{
		so_default_state().event< msg_one >( m_working_mbox,
			[this]{ so_deregister_agent_coop_normally(); } );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< msg_one >( m_working_mbox );
		so_5::send< msg_one >( m_working_mbox );
		so_5::send< msg_one >( m_working_mbox );
	}

private :
	so_5::rt::mbox_t m_working_mbox;
};

void
do_test(
	const std::string & test_name,
	std::function< void(a_test_t &) > test_tuner )
{
	try
	{
		run_with_time_limit(
			[test_tuner]()
			{
				so_5::launch(
						[test_tuner]( so_5::rt::environment_t & env )
						{
							auto coop = env.create_coop( so_5::autoname );
							auto agent = coop->make_agent< a_test_t >();

							test_tuner( *agent );

							env.register_coop( std::move( coop ) );
						} );
			},
			20,
			test_name );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		std::abort();
	}
}

