/*
 * A test for attempt to subscribe to a mutable signal.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std;

void make_coop( so_5::environment_t & env )
{
	struct hello final : public so_5::signal_t {};

	env.introduce_coop( []( so_5::coop_t & coop ) {
		auto a = coop.define_agent();
		a.event< so_5::mutable_msg<hello> >( a, []{} );
	} );
}

int
main()
{
	return 0;
}

