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
	struct msg_1 {};
	struct msg_2 {};
	struct done {};

	so_5::wrapped_env_t env;

	auto params = build_mchain_params();

	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		auto ch1 = env.environment().create_mchain( p.second );

		so_5::send< msg_1 >( ch1 );
		so_5::send< msg_2 >( ch1 );
		so_5::send< done >( ch1 );

		auto r = so_5::select(
				so_5::from_all()
					.handle_all()
					.total_time( std::chrono::seconds{1} )
					.no_wait_on_empty(),
				receive_case( ch1, []( done ) {} )
			);

		UT_CHECK_EQ( std::size_t{3}, r.extracted() );
		UT_CHECK_EQ( std::size_t{1}, r.handled() );
		UT_CHECK_EQ( std::size_t{0}, r.sent() );
	}
}

int
main()
{
	try
	{
		run_with_time_limit(
			do_test,
			20 );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

