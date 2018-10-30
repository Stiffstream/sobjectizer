/*
 * A simplest test for enveloped message.
 */

#include <so_5/all.hpp>
#include <so_5/h/stdcpp.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

class test_envelope_t final : public so_5::enveloped_msg::envelope_t
{
	std::mutex m_lock;

	so_5::outliving_reference_t< std::string > m_receiver;
	const std::string m_id;

	so_5::message_ref_t m_payload;

	void
	append_text( const char * what )
	{
		std::lock_guard< std::mutex > lock{ m_lock };
		m_receiver.get() += m_id;
		m_receiver.get() += ":";
		m_receiver.get() += what;
	}

public:
	test_envelope_t(
		so_5::outliving_reference_t< std::string > receiver,
		std::string id,
		so_5::message_ref_t payload )
		:	m_receiver{ receiver }
		,	m_id( std::move(id) )
		,	m_payload{ std::move(payload) }
		{}

	void
	access_hook(
		access_context_t context,
		handler_invoker_t & invoker ) SO_5_NOEXCEPT override
	{
		switch( context )
		{
		case access_context_t::handler_found:
			append_text( "pre_invoke;" );
			invoker.invoke( so_5::enveloped_msg::payload_info_t{ m_payload } );
			append_text( "post_invoke;" );
		break;

		case access_context_t::transformation:
			append_text( "transform;" );
			invoker.invoke( payload_info_t{ m_payload } );
		break;

		case access_context_t::inspection:
			append_text( "inspect;" );
			invoker.invoke( payload_info_t{ m_payload } );
		break;
		}
	}
};

template< typename Msg, typename... Args >
void
post_enveloped(
	std::string & receiver,
	std::string id,
	const so_5::mbox_t & mbox,
	Args && ...args )
{
	so_5::message_ref_t msg{
			so_5::stdcpp::make_unique<Msg>( std::forward<Args>(args)... ) };
	so_5::message_ref_t enveloped{
			so_5::stdcpp::make_unique<test_envelope_t>(
					so_5::outliving_mutable(receiver),
					std::move(id),
					std::move(msg) ) };

	mbox->do_deliver_enveloped_msg(
			so_5::message_payload_type<Msg>::subscription_type_index(),
			std::move(enveloped),
			1 );
}

struct msg_one final : public so_5::message_t {
	std::string m_value;

	msg_one( std::string value ) : m_value( std::move(value) ) {}
};

struct msg_two final : public so_5::message_t {
	std::string m_value;

	msg_two( std::string value ) : m_value( std::move(value) ) {}
};

struct shutdown final : public so_5::signal_t {};

class mpmc_mbox_case_t final : public so_5::agent_t
{
	const so_5::mbox_t m_mbox;
	std::string & m_receiver;

	state_t st_dummy{ this };

public:
	mpmc_mbox_case_t( context_t ctx, std::string & receiver )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_mbox{ so_environment().create_mbox() }
		,	m_receiver( receiver )
	{
		so_default_state().event( m_mbox,
				[this]( mhood_t<msg_one> cmd ) {
					m_receiver += "msg_one{" + cmd->m_value + "};";
				} )
			.event(
				[this]( mhood_t<shutdown> ) {
					so_deregister_agent_coop_normally();
				} );
		st_dummy.event( m_mbox,
				[this]( mhood_t<msg_two> cmd ) {
					m_receiver += "msg_two{" + cmd->m_value + "};";
				} );
	}

	void
	so_evt_start() override
	{
		post_enveloped<msg_one>( m_receiver, "1", m_mbox, "One" );
		post_enveloped<msg_two>( m_receiver, "2", m_mbox, "Two" );
		so_5::send<shutdown>( so_direct_mbox() );
	}
};

class direct_mbox_case_t final : public so_5::agent_t
{
	std::string & m_receiver;

public:
	direct_mbox_case_t( context_t ctx, std::string & receiver )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_receiver( receiver )
	{
		so_default_state().event(
				[this]( mhood_t<msg_one> cmd ) {
					m_receiver += "msg_one{" + cmd->m_value + "};";
				} )
			.event(
				[this]( mhood_t<shutdown> ) {
					so_deregister_agent_coop_normally();
				} );
	}

	void
	so_evt_start() override
	{
		post_enveloped<msg_one>( m_receiver, "1", so_direct_mbox(), "One" );
		post_enveloped<msg_two>( m_receiver, "2", so_direct_mbox(), "Two" );
		so_5::send<shutdown>( so_direct_mbox() );
	}
};

class direct_mbox_with_limits_case_t final : public so_5::agent_t
{
	std::string & m_receiver;

public:
	direct_mbox_with_limits_case_t( context_t ctx, std::string & receiver )
		:	so_5::agent_t{ ctx
				+ limit_then_abort<msg_one>(1)
				+ limit_then_abort<msg_two>(1)
				+ limit_then_abort<shutdown>(1) }
		,	m_receiver( receiver )
	{
		so_default_state().event(
				[this]( mhood_t<msg_one> cmd ) {
					m_receiver += "msg_one{" + cmd->m_value + "};";
				} )
			.event(
				[this]( mhood_t<shutdown> ) {
					so_deregister_agent_coop_normally();
				} );
	}

	void
	so_evt_start() override
	{
		post_enveloped<msg_one>( m_receiver, "1", so_direct_mbox(), "One" );
		post_enveloped<msg_two>( m_receiver, "2", so_direct_mbox(), "Two" );
		so_5::send<shutdown>( so_direct_mbox() );
	}
};
template< typename Test_Agent >
void
run_test( const char * test_case_name )
{
	std::cout << "=== " << test_case_name << " ===" << std::endl;

	std::string trace;
	so_5::launch( [&]( so_5::environment_t & env ) {
			env.introduce_coop( [&]( so_5::coop_t & coop ) {
					coop.make_agent< Test_Agent >( trace );
				} );
		} );

	std::cout << "trace is: " << trace << std::endl;

	const std::string expected =
			"1:pre_invoke;msg_one{One};1:post_invoke;";

	ensure_or_die(
			expected == trace,
			"trace missmatch:\n"
			"  actual=" + trace + "\n" 
			"expected=" + expected );

	std::cout << "DONE" << std::endl;
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_test< mpmc_mbox_case_t >( "mpmc_mbox" );
				run_test< direct_mbox_case_t >( "direct_mbox[no-limits]" );
				run_test< direct_mbox_with_limits_case_t >(
						"direct_mbox[with-limits]" );
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

