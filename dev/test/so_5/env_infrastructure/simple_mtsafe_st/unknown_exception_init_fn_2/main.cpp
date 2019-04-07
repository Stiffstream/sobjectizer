/*
 * A test for simple_mtsafe_st_env_infastructure with unknown
 * exception from init_fn function.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

using namespace std;

class coop_resource_t
{
	so_5::atomic_counter_t & m_counter;

public :
	coop_resource_t( so_5::atomic_counter_t & counter )
		:	m_counter{ counter }
	{
		++m_counter;
	}

	~coop_resource_t() noexcept
	{
		--m_counter;
	}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::atomic_counter_t counter{ 0 };

				try
				{
					so_5::launch(
						[&]( so_5::environment_t & env ) {
							// Create a bunch of empty coops.
							for( int i = 0; i != 10000; ++i )
							{
								auto coop = env.make_coop();
								coop->take_under_control(
										std::make_unique< coop_resource_t >(
												std::ref(counter) ) );

								auto id = env.register_coop( std::move(coop) );

								// Some of them will be deregistered.
								if( 0 == (i % 3) )
									env.deregister_coop( id,
											so_5::dereg_reason::normal );
							}

							throw "boom!";
						},
						[]( so_5::environment_params_t & params ) {
							params.infrastructure_factory(
									so_5::env_infrastructures::simple_mtsafe::factory() );
						} );

					// An exception should be thrown from so_5::launch.
					std::cout << "We expect an exception from launch";
					std::abort();
				}
				catch( const so_5::exception_t & x )
				{
					std::cout << "Exception is caught: " << x.what() << std::endl;
				}

				const auto actual_counter = counter.load();
				ensure_or_die( 0u == actual_counter,
						"expected value of counter is 0, actual=" +
						std::to_string(actual_counter) );
			},
			5 );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

