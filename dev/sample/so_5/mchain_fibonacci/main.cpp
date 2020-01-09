/*
 * Usage of select() with send_case() for calculation of Fibonacci numbers.
 */

#include <so_5/all.hpp>

#include <chrono>

using namespace std;
using namespace std::chrono_literals;
using namespace so_5;

struct quit {};

void fibonacci( mchain_t values_ch, mchain_t quit_ch )
{
	int x = 0, y = 1;
	mchain_select_result_t r;
	do
	{
		r = select(
			from_all().handle_n(1),
			send_case( values_ch, message_holder_t<int>::make(x),
					[&x, &y] { 
						auto old_x = x;
						x = y; y = old_x + y;
					} ),
			receive_case( quit_ch, [](quit){} ) );
	}
	while( r.was_sent() && !r.was_handled() );
}

int main()
{
	wrapped_env_t sobj;

	thread fibonacci_thr;
	auto thr_joiner = auto_join( fibonacci_thr );

	auto values_ch = create_mchain( sobj, 1s, 1,
			mchain_props::memory_usage_t::preallocated,
			mchain_props::overflow_reaction_t::abort_app );

	auto quit_ch = create_mchain( sobj );
	auto ch_closer = auto_close_drop_content( values_ch, quit_ch );

	fibonacci_thr = thread{ fibonacci, values_ch, quit_ch };

	receive( from( values_ch ).handle_n( 10 ), []( int v ) { cout << v << endl; } );

	send< quit >( quit_ch );
}

