/*
 * Test for closing mchain which is used in different thread in
 * different selects.
 *
 * If `select` waits on send_case than closing the chain from
 * receive_case doesn't break the `select`.
 */

#include <so_5/all.hpp>

#include <string>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include "../mchain_params.hpp"

using namespace std;

struct ready {};
struct finished {};

void
worker_thread(
	so_5::mchain_t ready_ch,
	so_5::mchain_t in_ch,
	so_5::mchain_t out_ch)
{
	so_5::send< ready >( ready_ch );
	so_5::select(
			so_5::from_all().handle_n(1),
			send_case( out_ch, so_5::message_holder_t<int>::make(0), []() {}),
			receive_case( in_ch, [](int) {} ) );
	so_5::send< finished >( ready_ch );
}

so_5::mchain_params_t
mchain_params()
{
	return so_5::make_unlimited_mchain_params();
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::wrapped_env_t env;

				auto ready_ch = env.environment().create_mchain( mchain_params() );
				auto out_ch = so_5::create_mchain(env,
						std::chrono::seconds{ 1 },
						1,
						so_5::mchain_props::memory_usage_t::preallocated,
						so_5::mchain_props::overflow_reaction_t::abort_app);
				auto in_ch = env.environment().create_mchain( mchain_params() );

				// out_ch has to be full to avoid send_case().
				so_5::send<int>(out_ch, 0);

				thread w1{ worker_thread, ready_ch, in_ch, out_ch };
				thread w2{ worker_thread, ready_ch, in_ch, out_ch };
				thread w3{ worker_thread, ready_ch, in_ch, out_ch };

				receive( from( ready_ch ).handle_n( 3 ), []( ready ) {} );
				this_thread::sleep_for( chrono::milliseconds(500) );

				close_drop_content( so_5::exceptions_enabled, in_ch );

				// We shouldn't receive any `finished` signals here.
				const auto r1 = receive(
						from( ready_ch )
							.handle_n( 3 )
							.total_time( std::chrono::milliseconds{ 300 } ),
						[]( finished ) {} );
				ensure_or_die(0 == r1.handled(),
						"no `finished` singals has to be received");

				close_drop_content( so_5::exceptions_enabled, out_ch );

				const auto r2 = receive(
						from( ready_ch ).handle_n( 3 ), []( finished ) {} );
				ensure_or_die(3 == r2.handled(),
						"all `finished` singals have to be received");

				w1.join();
				w2.join();
				w3.join();
			},
			2,
			"close mchain (select in different threads)" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

