/*
 * Test for size-limited mchains with abort_app overflow reaction.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;
using namespace chrono;

namespace props = so_5::mchain_props;

int
main()
{
	so_5::wrapped_env_t env;

	auto ch = env.environment().create_mchain(
			so_5::make_limited_without_waiting_mchain_params(
					3,
					props::memory_usage_t::dynamic,
					props::overflow_reaction_t::abort_app ) );

	so_5::send< int >( ch, 1 );
	so_5::send< int >( ch, 2 );
	so_5::send< int >( ch, 3 );
	so_5::send< int >( ch, 4 );

	return 0;
}

