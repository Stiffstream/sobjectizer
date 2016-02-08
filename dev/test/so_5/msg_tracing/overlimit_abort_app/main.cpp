/*
 * A simple test for message delivery tracing in overlimit action abort_app.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
{
	struct dummy_msg { int m_i; };

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx + limit_then_abort< dummy_msg >( 1 ) }
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_test_t::evt_dummy_msg );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< dummy_msg >( *this, 1 );
		so_5::send< dummy_msg >( *this, 2 );
	}

private :
	void
	evt_dummy_msg( const dummy_msg & msg )
	{
		std::cout << "message received: " << msg.m_i << std::endl;
	}
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop( []( so_5::coop_t & coop ) {
			coop.make_agent< a_test_t >();
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init,
					[]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					} );
			},
			20,
			"simple tracing for overlimit reaction abort_app" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

