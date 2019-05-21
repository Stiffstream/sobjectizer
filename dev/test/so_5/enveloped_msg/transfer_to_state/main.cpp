/*
 * Check for enveloped message and transfer_to_state.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include "../common_stuff.hpp"

struct shutdown final {
	std::string m_msg;
};

class test_case_t final : public so_5::agent_t
{
	trace_t m_trace;
	const so_5::mbox_t m_mbox;

	state_t st_one{ this, "one" };
	state_t st_two{ this, "two" };

public :
	test_case_t( context_t ctx )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_mbox{ special_mbox_t<>::make(
				so_environment().create_mbox(),
				so_5::outliving_mutable(m_trace),
				"mbox" ) }
	{
		st_one.transfer_to_state< shutdown >( m_mbox, st_two );

		st_two.event( m_mbox, [this](mhood_t<shutdown> cmd) {
			ensure_or_die( "bye" == cmd->m_msg,
					"unexpected message inside shutdown: " + cmd->m_msg );
			so_deregister_agent_coop_normally();
		} );
	}

	void
	so_evt_start() override
	{
		this >>= st_one;

		so_5::send< shutdown >( m_mbox, "bye" );
	}

	void
	so_evt_finish() override
	{
		std::cout << "Trace is: " << m_trace.content() << std::endl;
		ensure_or_die( !m_trace.content().empty(),
				"trace should not be empty!" );
	}
};

void
run_test()
{
	so_5::launch( [&]( so_5::environment_t & env ) {
			env.introduce_coop( [&]( so_5::coop_t & coop ) {
					coop.make_agent< test_case_t >();
				} );
		},
		[]( so_5::environment_params_t & params ) {
			params.message_delivery_tracer(
					so_5::msg_tracing::std_cout_tracer() );
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_test();
			},
			5 );
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

