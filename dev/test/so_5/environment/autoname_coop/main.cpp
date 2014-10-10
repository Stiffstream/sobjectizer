/*
 * A simple test for checking autoname feature of SO Environment.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::rt::agent_t
{
	public:
		a_test_t( so_5::rt::environment_t & env )
			:	so_5::rt::agent_t( env )
		{}

		virtual void
		so_evt_start()
		{
			so_deregister_agent_coop_normally();
		}
};

int
main( int argc, char * argv[] )
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch(
					[]( so_5::rt::environment_t & env )
					{
						for( int i = 0; i < 1024; ++i )
						{
							auto coop = env.create_coop( so_5::autoname );
							coop->add_agent( new a_test_t( env ) );

							env.register_coop( std::move( coop ) );
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

