/*
 * A test for using direct mbox with limits and another mbox MPSC
 * mbox without limits.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <so_5/impl/internal_env_iface.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

struct msg_timeout : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	const so_5::mbox_t m_another_mbox;

	bool m_second_msg_timeout_received{ false };

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx + limit_then_abort< msg_timeout >( 1 ) )
		,	m_another_mbox{
				so_5::impl::internal_env_iface_t{ so_environment() }
						.create_limitless_mpsc_mbox( *this )
			}
	{}

	void
	so_define_agent() override
	{
		so_default_state()
			// Message from the standard direct mbox.
			.event( [this](mhood_t< msg_timeout >) {
					so_deregister_agent_coop_normally();
				} )
			// Message from manually created MPSC mbox.
			.event( m_another_mbox, [this](mhood_t< msg_timeout >) {
					m_second_msg_timeout_received = true;
				} )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send_delayed< msg_timeout >( *this, std::chrono::milliseconds{50} );
		so_5::send_delayed< msg_timeout >( m_another_mbox, std::chrono::milliseconds{70} );

		std::this_thread::sleep_for( std::chrono::milliseconds{150} );
	}

	void
	so_evt_finish() override
	{
		ensure_or_die( m_second_msg_timeout_received,
				"second msg_timeout wasn't received!" );
	}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( []( so_5::environment_t & env ) {
						env.register_agent_as_coop( env.make_agent< a_test_t >() );
					} );
			},
			5 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

