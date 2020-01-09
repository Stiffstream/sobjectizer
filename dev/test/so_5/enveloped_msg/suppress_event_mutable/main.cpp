/*
 * Check for enveloped message and state.suppress (for mutable msg).
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include "../common_stuff.hpp"

struct hello final
{
	std::string m_msg;
};

struct stop final : public so_5::signal_t {};

class test_case_t final : public so_5::agent_t
{
	trace_t m_trace;
	const so_5::mbox_t m_mbox;

	state_t st_parent{ this, "parent" };
	state_t st_child{ initial_substate_of{ st_parent }, "child" };

public :
	test_case_t( context_t ctx )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_mbox{ special_mbox_t<>::make(
				so_make_new_direct_mbox(),
				so_5::outliving_mutable(m_trace),
				"mbox" ) }
	{
		st_parent
			.event( [this](mhood_t<stop>) {
					so_deregister_agent_coop_normally();
				} )
			.event( m_mbox, [](mhood_t<hello> cmd) {
					ensure_or_die( "hello-immutable" == cmd->m_msg,
							"unexpected value of hello: " + cmd->m_msg );
				} )
			.event( m_mbox, [](mutable_mhood_t<hello>) {
					std::cout << "event handler for mutable 'hello' in st_parent "
						"must not be called!" << std::endl;
					std::abort();
				} );

		st_child.suppress< so_5::mutable_msg<hello> >( m_mbox );
	}

	void
	so_evt_start() override
	{
		this >>= st_parent;

		so_5::send< so_5::mutable_msg<hello> >( m_mbox, "hello" );
		so_5::send< hello >( m_mbox, "hello-immutable" );
		so_5::send< stop >( *this );
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

