/*
 * A test for insertion of a message into closed chain.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

int
main()
{
	try
	{
	run_with_time_limit(
		[]()
		{
			struct hello : public so_5::signal_t {};

			so_5::wrapped_env_t env;

			auto ch = create_mchain( env,
					std::chrono::seconds{45},
					2,
					so_5::mchain_props::memory_usage_t::dynamic,
					so_5::mchain_props::overflow_reaction_t::remove_oldest );

			std::thread worker{ [ch] {
				so_5::send< hello >( ch );
				so_5::send< hello >( ch );
				// This send should be blocked.
				so_5::send< hello >( ch );
			} };

			while( 2u != ch->size() )
				std::this_thread::sleep_for( std::chrono::milliseconds{25} );

			close_drop_content( ch );

			worker.join();

			UT_CHECK_CONDITION( 0u == ch->size() );
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

