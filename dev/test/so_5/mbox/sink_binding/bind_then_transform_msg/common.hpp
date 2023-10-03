#pragma once

#include <iostream>

#include <so_5/bind_then_transform_helpers.hpp>
#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test {

struct msg_src_1 final : public so_5::message_t
	{
		int m_a;
		int m_b;
		int m_c;

		msg_src_1( int a, int b, int c )
			: m_a{ a }, m_b{ b }, m_c{ c }
			{}
	};

struct msg_res_1 final : public so_5::message_t
	{
		std::string m_v;

		explicit msg_res_1( std::string v ) : m_v{ std::move(v) }
			{}
	};

struct msg_src_2
	{
		int m_a;
		int m_b;
		int m_c;

		msg_src_2( int a, int b, int c )
			: m_a{ a }, m_b{ b }, m_c{ c }
			{}
	};

struct msg_res_2
	{
		std::string m_v;

		explicit msg_res_2( std::string v ) : m_v{ std::move(v) }
			{}
	};

struct msg_complete final : public so_5::signal_t {};

template< typename Msg >
struct mhood_selector_t
	{
		using type = so_5::mhood_t<Msg>;
	};

template< typename Msg >
struct mhood_selector_t< so_5::mutable_msg<Msg> >
	{
		using type = so_5::mutable_mhood_t<Msg>;
	};

template< typename Msg >
using mhood_from_param_t = typename mhood_selector_t<Msg>::type;

template< typename Msg >
struct dr_param_selector_t
	{
		using type = Msg;
	};

template< typename Msg >
struct dr_param_selector_t< so_5::immutable_msg<Msg> >
	{
		using type = Msg;
	};

template< typename Msg >
struct dr_param_selector_t< so_5::mutable_msg<Msg> >
	{
		using type = Msg;
	};

template< typename Msg >
using dr_param_from_source_msg_t = typename dr_param_selector_t<Msg>::type;

template< typename Result_Msg, typename Test_Case_Handler >
class a_receiver_t final : public so_5::agent_t
	{
		std::string m_log;

	public:
		a_receiver_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
			{}

		void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( &a_receiver_t::evt_res )
					.event( &a_receiver_t::evt_complete )
					;
			}

	private:
		void
		evt_res( mhood_from_param_t<Result_Msg> cmd )
			{
				m_log += cmd->m_v;
				m_log += ';';
			}

		void
		evt_complete( so_5::mhood_t<msg_complete> )
			{
				Test_Case_Handler::check_result( m_log );
				so_deregister_agent_coop_normally();
			}
	};

template<
	typename Binding,
	typename Source_Msg,
	typename Result_Msg,
	typename Test_Case_Handler >
class a_sender_t final : public so_5::agent_t
	{
		const so_5::mbox_t m_receiver;

	public:
		a_sender_t( context_t ctx, so_5::mbox_t receiver )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_receiver{ std::move(receiver) }
			{}

		void
		so_evt_start() override
		{
			Binding binding;

			const auto src_mbox = so_5::make_unique_subscribers_mbox(
					so_environment() );

			Test_Case_Handler::template tune_binding< Source_Msg, Result_Msg >(
					binding, src_mbox, m_receiver );

			so_5::send< Source_Msg >( src_mbox, 1, 2, 3 );
			so_5::send< Source_Msg >( src_mbox, 2, 3, 4 );
			so_5::send< Source_Msg >( src_mbox, 3, 4, 5 );
			so_5::send< Source_Msg >( src_mbox, 4, 5, 6 );

			so_5::send< msg_complete >( m_receiver );
		}
	};

template<
	typename Binding,
	typename Source_Msg,
	typename Result_Msg,
	typename Test_Case_Handler >
void
run_test_case()
	{
		using receiver_t =
				a_receiver_t< Result_Msg, Test_Case_Handler >;
		using sender_t =
				a_sender_t< Binding, Source_Msg, Result_Msg, Test_Case_Handler >;

		std::cout << "running test case:"
				<< "\n  Binding: " << typeid(Binding).name()
				<< "\n  Source : " << typeid(Source_Msg).name()
				<< "\n  Result : " << typeid(Result_Msg).name()
				<< "\n  Name   : " << Test_Case_Handler::name()
				<< "\n  ...    : " << std::flush;

		run_with_time_limit(
			[]()
			{
				so_5::launch(
					[]( so_5::environment_t & env )
					{
						env.introduce_coop( []( so_5::coop_t & coop ) {
								auto * receiver = coop.make_agent< receiver_t >();
								coop.make_agent< sender_t >( receiver->so_direct_mbox() );
							} );
					} );
			},
			5 );

		std::cout << "OK" << std::endl;
	}

inline void
ensure_valid_or_die(
	const std::string_view case_name,
	const std::string_view expected,
	const std::string_view actual )
	{
		ensure_or_die( expected == actual,
				std::string{ case_name } + ": expected='"
				+ std::string{ expected }
				+ "', actual='" + std::string{ actual } + "'" );
	}

template<
	typename Source_Msg,
	typename Result_Msg,
	typename Test_Case_Handler >
void
run_test_case_for_msg_pair()
	{
		constexpr bool is_applicable_to_implicit_case =
				std::is_same_v<
						typename so_5::message_payload_type< Source_Msg >::payload_type,
						Source_Msg >;

		if constexpr(
				( Test_Case_Handler::is_implicit && is_applicable_to_implicit_case )
				|| !Test_Case_Handler::is_implicit )
			{
				run_test_case<
						so_5::single_sink_binding_t,
						Source_Msg, Result_Msg,
						Test_Case_Handler >();
				run_test_case<
						so_5::multi_sink_binding_t<>,
						Source_Msg, Result_Msg,
						Test_Case_Handler >();
			}
	}

template< typename Test_Case_Handler >
void
run_tests_for_case_handler()
	{
		// msg_src_1, msg_res_1
		//
		run_test_case_for_msg_pair< msg_src_1, msg_res_1, Test_Case_Handler >();

		// msg_src_2, msg_res_1
		//
		run_test_case_for_msg_pair< msg_src_2, msg_res_1, Test_Case_Handler >();

		// msg_src_2, msg_res_2
		//
		run_test_case_for_msg_pair< msg_src_2, msg_res_2, Test_Case_Handler >();

		// msg_src_1, msg_res_2
		//
		run_test_case_for_msg_pair< msg_src_1, msg_res_2, Test_Case_Handler >();

		// so_5::immutable_msg.

		// msg_src_1, msg_res_1
		//
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_1>,
				msg_res_1,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				msg_src_1,
				so_5::immutable_msg<msg_res_1>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_1>,
				so_5::immutable_msg<msg_res_1>,
				Test_Case_Handler >();

		// msg_src_2, msg_res_1
		//
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_2>,
				msg_res_1,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				msg_src_2,
				so_5::immutable_msg<msg_res_1>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_2>,
				so_5::immutable_msg<msg_res_1>,
				Test_Case_Handler >();

		// msg_src_2, msg_res_2
		//
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_2>,
				msg_res_2,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				msg_src_2,
				so_5::immutable_msg<msg_res_2>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_2>,
				so_5::immutable_msg<msg_res_2>,
				Test_Case_Handler >();

		// msg_src_1, msg_res_2
		//
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_1>,
				msg_res_2,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				msg_src_1,
				so_5::immutable_msg<msg_res_2>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_1>,
				so_5::immutable_msg<msg_res_2>,
				Test_Case_Handler >();

		// so_5::mutable_msg.

		// msg_src_1, msg_res_1
		//
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_1>,
				msg_res_1,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				msg_src_1,
				so_5::mutable_msg<msg_res_1>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_1>,
				so_5::mutable_msg<msg_res_1>,
				Test_Case_Handler >();

		// msg_src_2, msg_res_1
		//
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_2>,
				msg_res_1,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				msg_src_2,
				so_5::mutable_msg<msg_res_1>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_2>,
				so_5::mutable_msg<msg_res_1>,
				Test_Case_Handler >();

		// msg_src_2, msg_res_2
		//
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_2>,
				msg_res_2,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				msg_src_2,
				so_5::mutable_msg<msg_res_2>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_2>,
				so_5::mutable_msg<msg_res_2>,
				Test_Case_Handler >();

		// msg_src_1, msg_res_2
		//
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_1>,
				msg_res_2,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				msg_src_1,
				so_5::mutable_msg<msg_res_2>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_1>,
				so_5::mutable_msg<msg_res_2>,
				Test_Case_Handler >();

		// so_5::mutable_msg + so_5::immutable_msg.

		// msg_src_1, msg_res_1
		//
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_1>,
				so_5::immutable_msg<msg_res_1>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_1>,
				so_5::mutable_msg<msg_res_1>,
				Test_Case_Handler >();

		// msg_src_2, msg_res_1
		//
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_2>,
				so_5::immutable_msg<msg_res_1>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_2>,
				so_5::mutable_msg<msg_res_1>,
				Test_Case_Handler >();

		// msg_src_2, msg_res_2
		//
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_2>,
				so_5::immutable_msg<msg_res_2>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_2>,
				so_5::mutable_msg<msg_res_2>,
				Test_Case_Handler >();

		// msg_src_1, msg_res_2
		//
		run_test_case_for_msg_pair<
				so_5::mutable_msg<msg_src_1>,
				so_5::immutable_msg<msg_res_2>,
				Test_Case_Handler >();
		run_test_case_for_msg_pair<
				so_5::immutable_msg<msg_src_1>,
				so_5::mutable_msg<msg_res_2>,
				Test_Case_Handler >();
	}

struct implicit_type_case_t
	{
		static constexpr bool is_implicit = true;
	};

struct explicit_type_case_t
	{
		static constexpr bool is_implicit = false;
	};

} /* namespace test */

