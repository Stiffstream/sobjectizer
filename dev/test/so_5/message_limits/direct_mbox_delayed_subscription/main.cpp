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

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct check final : public so_5::signal_t {};
struct subscribe final : public so_5::signal_t {};
struct shutdown final : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
public :
	a_test_t(
		so_5::environment_t & env )
		:	so_5::agent_t( env
				+ limit_then_abort< subscribe >( 1u )
				+ limit_then_drop< any_unspecified_message >( 2u )
				+ limit_then_abort< shutdown >( 1u ) )
	{}

	void
	so_define_agent() override
	{
		so_default_state()
			.event( [&](mhood_t< subscribe >) {
				so_subscribe_self().event( [this](mhood_t<check>) {
					++m_received;
				} );
			} )
			.event( [&](mhood_t< shutdown >) {
				if( 0u != m_received )
					throw std::runtime_error( "unexpected count of "
							"received 'check' instances: " +
							std::to_string( m_received ) );

				so_deregister_agent_coop_normally();
			} );
	}

	void
	so_evt_start() override
	{
		so_5::send< subscribe >( *this );

		// At this moment there is no subscription for check message.
		// All those messages should be ignored.
		so_5::send< check >( *this );
		so_5::send< check >( *this );
		so_5::send< check >( *this );
		so_5::send< check >( *this );
		so_5::send< check >( *this );

		so_5::send< shutdown >( *this );
	}

private :
	unsigned int m_received = 0;
};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( env.make_agent< a_test_t >() );
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
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

