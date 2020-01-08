/*
 * Check for enveloped message and transfer_to_state.
 * Another test case: handler for a message is not found in the
 * target state.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include "../common_stuff.hpp"

struct hello final {
	std::string m_msg;
};

struct shutdown final {
	std::string m_msg;
};

class test_case_t final : public so_5::agent_t
{
	trace_t m_trace_hello;
	const so_5::mbox_t m_mbox_hello;

	trace_t m_trace_shutdown;
	const so_5::mbox_t m_mbox_shutdown;

	state_t st_one{ this, "one" };
	state_t st_two{ this, "two" };

public :
	test_case_t( context_t ctx )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_mbox_hello{ special_mbox_t<>::make(
				so_environment().create_mbox(),
				so_5::outliving_mutable(m_trace_hello),
				"hello" ) }
		,	m_mbox_shutdown{ special_mbox_t<>::make(
				so_environment().create_mbox(),
				so_5::outliving_mutable(m_trace_shutdown),
				"shutdown" ) }
	{
		st_one.transfer_to_state< hello >( m_mbox_hello, st_two );
		// NOTE: there is no event handler for 'hello' in st_two.

		st_two.event( m_mbox_shutdown, [this](mhood_t<shutdown> cmd) {
			ensure_or_die( "bye" == cmd->m_msg,
					"unexpected message inside shutdown: " + cmd->m_msg );

			ensure_or_die( m_trace_hello.content().empty(),
					"content of m_trace_hello is expected to be empty: " +
					m_trace_hello.content() );

			so_deregister_agent_coop_normally();
		} );
	}

	void
	so_evt_start() override
	{
		this >>= st_one;

		so_5::send< hello >( m_mbox_hello, "hello" );
		so_5::send< shutdown >( m_mbox_shutdown, "bye" );
	}

	void
	so_evt_finish() override
	{
		std::cout << "Trace is: " << m_trace_shutdown.content() << std::endl;
		ensure_or_die( !m_trace_shutdown.content().empty(),
				"m_trace_shutdown should not be empty!" );
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

