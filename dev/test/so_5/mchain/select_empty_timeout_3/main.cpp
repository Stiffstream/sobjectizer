/*
 * Test for empty_timeout.
 */

#include <so_5/mchain_select.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include <test/so_5/mchain/mchain_params.hpp>

using namespace std;

template<typename T> struct debug;

void
do_test()
{
	struct hello {};
	struct to_be_ignored {};
	struct hello_again {};

	so_5::wrapped_env_t env;

	auto params = build_mchain_params();

	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		auto ch1 = env.environment().create_mchain( p.second );
		auto ch2 = env.environment().create_mchain( p.second );
		auto ch3 = env.environment().create_mchain( p.second );

		bool hello_received = false;
		bool hello_again_received = false;

		so_5::send< hello >( ch2 );

		auto r = so_5::select(
				so_5::from_all()
					.handle_all()
					.empty_timeout( std::chrono::milliseconds{ 50 } ),
				receive_case( ch1, []( hello ) {
						throw std::runtime_error( "hello from ch1!" );
					} ),
				receive_case( ch2,
					[&hello_received, ch2]( hello ) {
						hello_received = true;

						std::this_thread::sleep_for(
								std::chrono::milliseconds{ 100 } );

						so_5::send< to_be_ignored >( ch2 );
						so_5::send< hello_again >( ch2 );
					},
					[&hello_again_received]( hello_again ) {
						hello_again_received = true;

						std::this_thread::sleep_for(
								std::chrono::milliseconds{ 100 } );
					} ),
				receive_case( ch3, []( hello ) {
						throw std::runtime_error( "hello from ch3!" );
					} )
				);

		UT_CHECK_CONDITION( hello_received );
		UT_CHECK_CONDITION( hello_again_received );
		UT_CHECK_CONDITION( 3 == r.extracted() );
		UT_CHECK_CONDITION( 2 == r.handled() );
	}
}

int
main()
{
	try
	{
		run_with_time_limit(
			do_test,
			20,
			"test for simple multi chain select" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

