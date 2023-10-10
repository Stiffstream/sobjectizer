/*
 * Test for checking redirection deep for bind_transformer.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test {

struct msg_signal final : public so_5::signal_t {};

void
do_test( so_5::environment_t & env )
	{
		auto dest = env.create_mbox();
		so_5::single_sink_binding_t binding;
		so_5::bind_transformer< msg_signal >(
				binding,
				dest,
				[dest]() {
					return so_5::make_transformed< msg_signal >( dest );
				} );

		// This call should lead to infinite recursion if
		// redirection deep isn't controlled.
		so_5::send< msg_signal >( dest );
	}

} /* namespace test */

int
main()
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch(
					[]( so_5::environment_t & env )
					{
						test::do_test( env );
					} );
			},
			5 );

		return 0;
	}

