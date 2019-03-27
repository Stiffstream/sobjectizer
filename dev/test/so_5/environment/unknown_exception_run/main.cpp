/*
 * Test for unknown exception from init function.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

class test_agent_t final : public so_5::agent_t
{
public :
	test_agent_t( context_t ctx ) : agent_t{ std::move(ctx) } {}

	void
	so_evt_start() override
	{
		so_deregister_agent_coop_normally();
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
				try
				{
					so_5::launch( [&]( so_5::environment_t & env ) {
							env.register_agent_as_coop(
									env.make_agent< test_agent_t >() );

							throw( "boom!" );
						} );

					// An exception should be thrown from so_5::launch.
					std::cout << "We expect an exception from launch";
					std::abort();
				}
				catch( const so_5::exception_t & x )
				{
					std::cout << "Exception is caught: " << x.what() << std::endl;
				}
			},
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 2;
	}

	return 0;
}

