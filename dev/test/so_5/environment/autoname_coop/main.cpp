#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t( so_5::environment_t & env )
			:	so_5::agent_t( env )
		{}

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
				so_5::launch(
					[]( so_5::environment_t & env )
					{
						for( int i = 0; i < 1024; ++i )
						{
							auto coop = env.make_coop();
							coop->add_agent( std::make_unique< a_test_t >( env ) );

							env.register_coop( std::move( coop ) );

							env.register_agent_as_coop(
									std::make_unique< a_test_t >( env ) );

							env.register_agent_as_coop(
									std::make_unique< a_test_t >( env ),
									so_5::disp::one_thread::make_dispatcher(
											env, "another" ).binder() );

							std::cout << i << "\r" << std::flush;
						}

						std::cout << "Done." << std::flush;
					} );

				std::cout << " Completed." << std::endl;
			},
			600 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

