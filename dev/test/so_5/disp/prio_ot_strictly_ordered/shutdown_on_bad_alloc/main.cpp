/*
 * Test for a normal shutdown after std::bad_alloc.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include "../../custom_new_operator.hpp"

#include "../../shutdown_on_bad_alloc_common_stuff.cpp"

using namespace test;

int
main()
	{
		try
			{
				run_with_time_limit(
					[]()
					{
						using namespace so_5::disp::prio_one_thread::strictly_ordered;

						so_5::launch(
							[]( so_5::environment_t & env )
							{
								auto binder = make_dispatcher( env ).binder();
								env.register_agent_as_coop(
										env.make_agent< a_test_t >( binder ),
										binder );
							},
							[]( so_5::environment_params_t & params ) {
								params.exception_reaction(
										so_5::exception_reaction_t::shutdown_sobjectizer_on_exception );
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

