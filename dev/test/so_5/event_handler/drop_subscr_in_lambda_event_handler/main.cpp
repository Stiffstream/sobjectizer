/*
 * Test for calling so_drop_subscription in event-handler which is
 * a lambda-function.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
{
	struct test_signal : public so_5::signal_t {};
	struct shutdown : public so_5::signal_t {};

public :
	a_test_t( context_t ctx ) : so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self().event< shutdown >( [this] {
			so_deregister_agent_coop_normally();
		} );
	}

	virtual void
	so_evt_start() override
	{
		for( std::size_t i = 0; i != 1000; ++i )
		{
			auto unique_mbox = so_environment().create_mbox();
			so_subscribe( unique_mbox ).event< test_signal >(
				[this, unique_mbox] {
					so_drop_subscription< test_signal >( unique_mbox );
				} );
			so_5::send< test_signal >( unique_mbox );
		}

		so_5::send< shutdown >( *this );
	}

private :
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
				so_5::launch( &init );
			},
			20,
			"so_drop_subscription in lambda event-handler" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

