/*
 * A simple test for checking autoname feature of SO Environment.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t( so_5::environment_t & env )
			:	so_5::agent_t( env )
		{}

		virtual void
		so_evt_start()
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
				so_5::launch(
					[]( so_5::environment_t & env )
					{
						for( int i = 0; i < 1024; ++i )
						{
							auto coop = env.create_coop( so_5::autoname );
							coop->add_agent( new a_test_t( env ) );

							env.register_coop( std::move( coop ) );

							env.register_agent_as_coop( so_5::autoname,
									new a_test_t( env ) );

							env.register_agent_as_coop( so_5::autoname,
									new a_test_t( env ),
									so_5::disp::one_thread::create_disp_binder(
											"another" ) );
						}
					},
					[]( so_5::environment_params_t & params )
					{
						params.add_named_dispatcher( "another",
							so_5::disp::one_thread::create_disp() );
					} );
			},
			20,
			"SO Environment autoshutdown test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

