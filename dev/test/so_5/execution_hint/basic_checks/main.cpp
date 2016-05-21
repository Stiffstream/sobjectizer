/*
 * Test for basic operations with execution hints.
 */
#define SO_5_EXECUTION_HINT_UNIT_TEST

#include <utest_helper_1/h/helper.hpp>

#include <so_5/all.hpp>

class test_environment_t
	:	public so_5::environment_t
	{
	public :
		test_environment_t()
			:	so_5::environment_t(
					so_5::environment_params_t() )
			{}

		virtual void
		init()
		{}
	};

struct msg_signal : public so_5::signal_t {};
struct msg_thread_safe_signal : public so_5::signal_t {};
struct msg_get_status : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
	{
	public :
		a_test_t( so_5::environment_t & env )
			:	so_5::agent_t( env )
			,	m_signal_handled( false )
			,	m_thread_safe_signal_handled( false )
			,	m_get_status_handled( false )
			{}

		void
		evt_signal()
			{
				m_signal_handled = true;
			}

		void
		evt_thread_safe_signal()
			{
				m_thread_safe_signal_handled = true;
			}

		std::string
		evt_get_status()
			{
				m_get_status_handled = true;
				return "OK";
			}

		bool m_signal_handled;
		bool m_thread_safe_signal_handled;
		bool m_get_status_handled;
	};

UT_UNIT_TEST( no_handlers )
{
	using namespace so_5;

	test_environment_t env;

	a_test_t agent( env );

	{
		execution_demand_t demand(
				&agent,
				message_limit::control_block_t::none(),
				0,
				typeid(msg_signal),
				message_ref_t(),
				agent_t::get_demand_handler_on_message_ptr() );

		auto hint = agent_t::so_create_execution_hint( demand );

		UT_CHECK_EQ( false, hint );
	}

	{
		execution_demand_t demand(
				&agent,
				message_limit::control_block_t::none(),
				0,
				typeid(msg_signal),
				message_ref_t(),
				agent_t::get_service_request_handler_on_message_ptr() );

		auto hint = agent_t::so_create_execution_hint( demand );

		UT_CHECK_EQ( true, hint );
	}

	{
		execution_demand_t demand(
				&agent,
				message_limit::control_block_t::none(),
				0,
				typeid(msg_signal),
				message_ref_t(),
				agent_t::get_demand_handler_on_start_ptr() );

		auto hint = agent_t::so_create_execution_hint( demand );

		UT_CHECK_EQ( true, hint );
	}

	{
		execution_demand_t demand(
				&agent,
				message_limit::control_block_t::none(),
				0,
				typeid(msg_signal),
				message_ref_t(),
				agent_t::get_demand_handler_on_finish_ptr() );

		auto hint = agent_t::so_create_execution_hint( demand );

		UT_CHECK_EQ( true, hint );
	}
}

UT_UNIT_TEST( event_handler )
{
	using namespace so_5;

	test_environment_t env;

	a_test_t agent( env );
	agent.so_subscribe( agent.so_direct_mbox() )
			.event( signal< msg_signal >, &a_test_t::evt_signal );
	agent.so_subscribe( agent.so_direct_mbox() )
			.event(
					signal< msg_thread_safe_signal >,
					&a_test_t::evt_thread_safe_signal,
					thread_safe );

	{
		execution_demand_t demand(
				&agent,
				message_limit::control_block_t::none(),
				agent.so_direct_mbox()->id(),
				typeid(msg_signal),
				message_ref_t(),
				agent_t::get_demand_handler_on_message_ptr() );

		auto hint = agent_t::so_create_execution_hint( demand );

		UT_CHECK_EQ( true, hint );
		UT_CHECK_EQ( false, hint.is_thread_safe() );

		UT_CHECK_EQ( false, agent.m_signal_handled );

		hint.exec( query_current_thread_id() );

		UT_CHECK_EQ( true, agent.m_signal_handled );
	}

	{
		execution_demand_t demand(
				&agent,
				message_limit::control_block_t::none(),
				agent.so_direct_mbox()->id(),
				typeid(msg_thread_safe_signal),
				message_ref_t(),
				agent_t::get_demand_handler_on_message_ptr() );

		auto hint = agent_t::so_create_execution_hint( demand );

		UT_CHECK_EQ( true, hint );
		UT_CHECK_EQ( true, hint.is_thread_safe() );

		UT_CHECK_EQ( false, agent.m_thread_safe_signal_handled );

		hint.exec( query_current_thread_id() );

		UT_CHECK_EQ( true, agent.m_thread_safe_signal_handled );
	}
}

UT_UNIT_TEST( service_handler )
{
	using namespace so_5;

	test_environment_t env;

	a_test_t agent( env );

	{
		std::promise< std::string > promise;
		auto f = promise.get_future();

		message_ref_t msg(
				new msg_service_request_t< std::string, msg_get_status >(
						std::move(promise) ) );

		execution_demand_t demand(
				&agent,
				message_limit::control_block_t::none(),
				agent.so_direct_mbox()->id(),
				typeid(msg_get_status),
				msg,
				agent_t::get_service_request_handler_on_message_ptr() );

		auto hint = agent_t::so_create_execution_hint( demand );

		UT_CHECK_EQ( true, hint );
		UT_CHECK_EQ( true, hint.is_thread_safe() );

		hint.exec( query_current_thread_id() );

		UT_CHECK_THROW( exception_t, f.get() );

		UT_CHECK_EQ( false, agent.m_get_status_handled );
	}

	agent.so_subscribe( agent.so_direct_mbox() )
			.event( signal< msg_get_status >, &a_test_t::evt_get_status );

	{
		std::promise< std::string > promise;
		auto f = promise.get_future();

		message_ref_t msg(
				new msg_service_request_t< std::string, msg_get_status >(
						std::move(promise) ) );

		execution_demand_t demand(
				&agent,
				message_limit::control_block_t::none(),
				agent.so_direct_mbox()->id(),
				typeid(msg_get_status),
				msg,
				agent_t::get_service_request_handler_on_message_ptr() );

		auto hint = agent_t::so_create_execution_hint( demand );

		UT_CHECK_EQ( true, hint );
		UT_CHECK_EQ( false, hint.is_thread_safe() );

		hint.exec( query_current_thread_id() );

		UT_CHECK_EQ( "OK", f.get() );
		UT_CHECK_EQ( true, agent.m_get_status_handled );
	}
}

int
main()
{
	UT_RUN_UNIT_TEST( no_handlers )
	UT_RUN_UNIT_TEST( event_handler )
	UT_RUN_UNIT_TEST( service_handler )

	return 0;
}

