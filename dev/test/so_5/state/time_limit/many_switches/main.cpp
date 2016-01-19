/*
 * A test case with many switches between states.
 * This test must not lead to memory consumption, data damages or
 * any other negative consequences.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t final : public so_5::agent_t
{
	state_t first{ this, "first" };
	state_t second{ this, "second" };
	state_t fatal{ this, "fatal" };

	struct change_state : public so_5::signal_t {};

public :
	a_test_t( context_t ctx, unsigned long long switch_count )
		:	so_5::agent_t{ ctx }
		,	m_switch_count{ switch_count }
	{
		first
			.time_limit( std::chrono::seconds{1}, fatal )
			.event< change_state >( [this] {
				try_change_state_to( second );
			} );

		second
			.event< change_state >( [this] {
				this >>= first;
				so_5::send< change_state >( *this );
			} );

		fatal
			.on_enter( [this]{
				std::cerr << "Agent is switched to the fatal state!" << std::endl;
				std::cerr << "Switch passed: " << m_switch_passed << std::endl;
				throw std::runtime_error( "Should not be in this state!" );
			} );
	}

	virtual void
	so_evt_start() override
	{
		m_started_at = std::chrono::steady_clock::now();

		do_switch( first );
	}

	virtual void
	so_evt_finish() override
	{
		std::cout << "Total switches: " << m_switch_passed << std::endl;
	}

private :
	const unsigned long long m_switch_count;
	unsigned long long m_switch_passed{ 0ull };

	std::chrono::steady_clock::time_point m_started_at;

	void
	do_switch( const so_5::state_t & to )
	{
		this >>= to;
		so_5::send< change_state >( *this );
	}

	void
	finish_work()
	{
		std::cout << "Work will be finished" << std::endl;
		so_deregister_agent_coop_normally();
	}

	void
	try_change_state_to( const so_5::state_t & to )
	{
		++m_switch_passed;

		if( 0ull != m_switch_count )
		{
			if( m_switch_passed >= m_switch_count )
				finish_work();
			else
				do_switch( to );
		}
		else
		{
			auto now = std::chrono::steady_clock::now();
			if( now > (m_started_at + std::chrono::seconds{1}) )
				finish_work();
			else
				do_switch( to );
		}
	}
};

int
main( int argc, char ** argv )
{
	try
	{
		unsigned long long switch_count = 0ull;
		if( 2 == argc )
		{
			switch_count = std::stoull( argv[ 1 ] );
			std::cout << "Expected switch count: " << switch_count << std::endl;
		}

		run_with_time_limit(
			[switch_count]()
			{
				so_5::launch( [=]( so_5::environment_t & env ) {
						env.introduce_coop( [=]( so_5::coop_t & coop ) {
								coop.make_agent< a_test_t >( switch_count );
							} );
					},
					[]( so_5::environment_params_t & params ) {
						params.timer_thread( so_5::timer_list_factory() );
					} );
			},
			86400,
			"test for many switches from state to state" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

