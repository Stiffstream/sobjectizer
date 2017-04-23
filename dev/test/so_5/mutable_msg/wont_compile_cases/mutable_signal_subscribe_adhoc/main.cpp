/*
 * A test for attempt to subscribe to a mutable signal.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

void make_coop( so_5::environment_t & env )
{
	struct hello final : public so_5::signal_t {};

	env.introduce_coop( []( so_5::coop_t & coop ) {
		auto a = coop.define_agent();
		a.event( a, []( so_5::mutable_mhood_t<hello> ) {} );
	} );
}

int
main()
{
	return 0;
}

