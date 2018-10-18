/*
 * Check for limit_then_transform for a normal case.
 */

#include <so_5/all.hpp>
#include <so_5/h/stdcpp.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../../common_stuff.hpp"


struct msg_one final : public so_5::message_t {
	std::string m_value;

	msg_one( std::string value ) : m_value( std::move(value) ) {}
};

struct msg_two final : public so_5::message_t {
	std::string m_value;

	msg_two( std::string value ) : m_value( std::move(value) ) {}
};

class service_t final : public so_5::agent_t
{
	so_5::outliving_reference_t< trace_t > m_trace;

	const so_5::mbox_t m_mbox;

public:
	service_t(
		context_t ctx,
		so_5::mbox_t mbox,
		so_5::outliving_reference_t< trace_t > trace )
		:	so_5::agent_t{ ctx
				+ limit_then_transform(0,
					[this]( const msg_one & src ) {
						return make_transformed< msg_two >(
								so_direct_mbox(),
								"[" + src.m_value + "]" );
					} )
				+ limit_then_abort<msg_two>(1) }
		,	m_trace{ trace }
		,	m_mbox{ std::move(mbox) }

	{
		so_default_state().event( m_mbox,
				[this]( mhood_t<msg_one> cmd ) {
					m_trace.get().append( "msg_one{" + cmd->m_value + "};" );
				} )
			.event(
				[this]( mhood_t<msg_two> cmd ) {
					m_trace.get().append( "msg_two{" + cmd->m_value + "};" );
				} );
	}
};

class producer_t final : public so_5::agent_t
{
	const so_5::mbox_t m_mbox;

public :
	producer_t( context_t ctx, so_5::mbox_t mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_mbox{ std::move(mbox) }
	{}

	void
	so_evt_start() override
	{
		try
		{
			so_5::request_value< void, msg_one >(
					m_mbox, so_5::infinite_wait, "Hello!" );
		}
		catch( const so_5::exception_t & x )
		{
			ensure_or_die(
					so_5::rc_svc_request_cannot_be_transfomred_on_overlimit ==
							x.error_code(),
					"rc_svc_request_cannot_be_transfomred_on_overlimit is expected" );
		}

		so_deregister_agent_coop_normally();
	}
};

void
run_test()
{
	trace_t trace;
	so_5::launch( [&]( so_5::environment_t & env ) {
			auto mbox = special_mbox_t<>::make(
				env.create_mbox(),
				so_5::outliving_mutable(trace),
				"mbox" );

			env.introduce_coop( [&]( so_5::coop_t & coop ) {
					coop.make_agent_with_binder< service_t >(
							so_5::disp::one_thread::create_private_disp(
									env )->binder(),
							mbox,
							so_5::outliving_mutable(trace) );

					coop.make_agent< producer_t >( mbox );
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

