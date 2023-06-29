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
						so_5::launch(
							[]( so_5::environment_t & env )
							{
								env.register_agent_as_coop(
										env.make_agent< a_test_t >(
												env.so_make_default_disp_binder() ) );
							},
							[]( so_5::environment_params_t & params ) {
								params.default_disp_params(
										so_5::disp::nef_one_thread::disp_params_t{} );

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

