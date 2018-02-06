/*
 * A test for making a subcription for agent from external entity.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

struct subscription_data_t
	{
		so_5::mbox_t m_mbox;
		const so_5::state_t * m_state;
		std::type_index m_subscription_type;
		so_5::event_handler_method_t m_handler;

		subscription_data_t(
			so_5::mbox_t mbox,
			const so_5::state_t & state,
			std::type_index subscription_type,
			so_5::event_handler_method_t handler )
			:	m_mbox( std::move(mbox) )
			,	m_state( &state )
			,	m_subscription_type( std::move(subscription_type) )
			,	m_handler( std::move(handler) )
			{}
	};

class one_shot_subscription_t
	{
		so_5::outliving_reference_t< so_5::agent_t > m_agent;
		std::vector< subscription_data_t > m_subscriptions;

	public :
		one_shot_subscription_t( so_5::agent_t & agent )
			:	m_agent( outliving_mutable(agent) )
			{}

		template< typename Handler >
		void
		add_handler(
			const so_5::mbox_t & mbox,
			const so_5::state_t & state,
			Handler && handler )
			{
				const auto user_handler_data = make_user_handler(
						std::forward<Handler>(handler) );
				so_5::details::event_subscription_helpers::ensure_handler_can_be_used_with_mbox(
						user_handler_data,
						mbox );

				const auto user_handler = user_handler_data.m_handler;

				so_5::event_handler_method_t actual_handler =
					[this, user_handler](
						so_5::invocation_type_t invoke_type,
						so_5::message_ref_t & msg )
					{
						drop_subscriptions();
						user_handler( invoke_type, msg );
					};

				m_subscriptions.emplace_back(
						mbox,
						state,
						user_handler_data.m_msg_type,
						std::move(actual_handler) );
			}

		void
		activate()
			{
				for( const auto & sd : m_subscriptions )
					m_agent.get().so_create_event_subscription(
							sd.m_mbox,
							sd.m_subscription_type,
							*(sd.m_state),
							sd.m_handler,
							so_5::thread_safety_t::unsafe );
			}

	private :
		template< typename Event_Method >
		typename std::enable_if<
				so_5::details::is_agent_method_pointer<Event_Method>::value,
				so_5::details::msg_type_and_handler_pair_t >::type
		make_user_handler( Event_Method method )
			{
				using namespace so_5::details::event_subscription_helpers;
				using pfn_traits = so_5::details::is_agent_method_pointer<Event_Method>;
				using agent_type = typename pfn_traits::agent_type;

				auto cast_result = get_actual_agent_pointer< agent_type >(
						m_agent.get() );

				return make_handler_with_arg_for_agent(
						cast_result, method );
			}

		template< typename Event_Lambda >
		typename std::enable_if<
				!so_5::details::is_agent_method_pointer<Event_Lambda>::value,
				so_5::details::msg_type_and_handler_pair_t >::type
		make_user_handler( Event_Lambda && lambda )
			{
				return so_5::handler( std::forward<Event_Lambda>(lambda) );
			}

		void
		drop_subscriptions() SO_5_NOEXCEPT
			{
				so_5::details::invoke_noexcept_code( [&] {
					for( const auto & sd : m_subscriptions )
						m_agent.get().so_destroy_event_subscription(
								sd.m_mbox,
								sd.m_subscription_type,
								*(sd.m_state) );
				} );
			}
	};

class a_test_t final : public so_5::agent_t
	{
	private :
		one_shot_subscription_t m_one_shot;
		so_5::outliving_reference_t< std::string > m_trace;

		struct demo_signal final : public so_5::signal_t {};

		struct finish_signal final : public so_5::signal_t {};

	public :
		a_test_t(
			context_t ctx,
			so_5::outliving_reference_t< std::string > trace )
			:	so_5::agent_t( std::move(ctx) )
			,	m_one_shot( *this )
			,	m_trace( trace )
			{}

		virtual void
		so_define_agent() override
			{
				so_subscribe_self().event( &a_test_t::on_finish );
			}

		virtual void
		so_evt_start() override
			{
				m_one_shot.add_handler(
						so_direct_mbox(),
						so_default_state(),
						&a_test_t::on_demo_signal );

				m_one_shot.activate();

				so_5::send<demo_signal>( *this );
				so_5::send<demo_signal>( *this );

				so_5::send<finish_signal>( *this );
			}

	private :
		void
		on_demo_signal(mhood_t<demo_signal>)
			{
				m_trace.get() += "demo;";
			}

		void
		on_finish(mhood_t<finish_signal>)
			{
				so_deregister_agent_coop_normally();
			}
	};

class a_test2_t final : public so_5::agent_t
	{
	private :
		one_shot_subscription_t m_one_shot;
		so_5::outliving_reference_t< std::string > m_trace;

		struct demo_signal final : public so_5::signal_t {};

		struct finish_signal final : public so_5::signal_t {};

	public :
		a_test2_t(
			context_t ctx,
			so_5::outliving_reference_t< std::string > trace )
			:	so_5::agent_t( std::move(ctx) )
			,	m_one_shot( *this )
			,	m_trace( trace )
			{}

		virtual void
		so_define_agent() override
			{
				so_subscribe_self().event( &a_test2_t::on_finish );
			}

		virtual void
		so_evt_start() override
			{
				m_one_shot.add_handler(
						so_direct_mbox(),
						so_default_state(),
						[this](mhood_t<demo_signal>) {
							m_trace.get() += "demo2;";
						} );

				m_one_shot.activate();

				so_5::send<demo_signal>( *this );
				so_5::send<demo_signal>( *this );

				so_5::send<finish_signal>( *this );
			}

	private :
		void
		on_finish(mhood_t<finish_signal>)
			{
				so_deregister_agent_coop_normally();
			}
	};

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				std::string trace1;
				std::string trace2;
				so_5::launch( [&]( so_5::environment_t & env ) {
						env.introduce_coop( [&]( so_5::coop_t & coop ) {
								coop.make_agent< a_test_t >(
										so_5::outliving_mutable(trace1) );
							} );
						env.introduce_coop( [&]( so_5::coop_t & coop ) {
								coop.make_agent< a_test2_t >(
										so_5::outliving_mutable(trace2) );
							} );
					} );

				ensure_or_die( trace1 == "demo;",
						"trace1 has unexpected value: " + trace1 );
				ensure_or_die( trace2 == "demo2;",
						"trace2 has unexpected value: " + trace2 );
			},
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

