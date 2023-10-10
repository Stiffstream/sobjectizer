/*
 * Test cases for bind_transformer and signals.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test {

struct msg_signal final : public so_5::signal_t {};

struct msg_res_1 final : public so_5::message_t
	{
		std::string m_v;

		explicit msg_res_1( std::string v ) : m_v{ std::move(v) }
			{}
	};

struct msg_res_2
	{
		std::string m_v;

		explicit msg_res_2( std::string v ) : m_v{ std::move(v) }
			{}
	};

struct msg_res_3 final : public so_5::signal_t {};

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
				Test_Case_Handler::on_res( m_log, cmd );
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

			Test_Case_Handler::template tune_binding< Result_Msg >(
					binding, src_mbox, m_receiver );

			so_5::send< msg_signal >( src_mbox );
			so_5::send< msg_signal >( src_mbox );
			so_5::send< msg_signal >( src_mbox );

			so_5::send< msg_complete >( m_receiver );
		}
	};

template<
	typename Binding,
	typename Result_Msg,
	typename Test_Case_Handler >
void
run_test_case()
	{
		using receiver_t =
				a_receiver_t< Result_Msg, Test_Case_Handler >;
		using sender_t =
				a_sender_t< Binding, Result_Msg, Test_Case_Handler >;

		std::cout << "running test case:"
				<< "\n  Binding: " << typeid(Binding).name()
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

void
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

struct explicit_type_no_optional_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "explicit_type_no_optional" }; }

		template< typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				so_5::bind_transformer< msg_signal >(
						binding,
						from,
						[to]() {
							return so_5::make_transformed< Result_Msg >( to, "T" );
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "T;T;T;", log );
			}

		template< typename Mhood >
		static void
		on_res( std::string & log, Mhood && cmd )
			{
				log += cmd->m_v;
			}
	};

struct explicit_type_with_optional_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "explicit_type_with_optional" }; }

		template< typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				using ret_val_t = std::optional< so_5::transformed_message_t< Result_Msg > >;
				so_5::bind_transformer< msg_signal >(
						binding,
						from,
						[to, counter=int{0}]() mutable -> ret_val_t {
							const auto c = counter++;
							if( 1 == c )
								return std::nullopt;

							return { so_5::make_transformed< Result_Msg >( to, "T" ) };
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "T;T;", log );
			}

		template< typename Mhood >
		static void
		on_res( std::string & log, Mhood && cmd )
			{
				log += cmd->m_v;
			}
	};

struct transform_to_singal_no_optional_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "transform_to_singal_no_optional" }; }

		template< typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				so_5::bind_transformer< msg_signal >(
						binding,
						from,
						[to]() {
							return so_5::make_transformed< Result_Msg >( to );
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "t;t;t;", log );
			}

		template< typename Mhood >
		static void
		on_res( std::string & log, Mhood && /*cmd*/ )
			{
				log += "t";
			}
	};

struct transform_to_singal_with_optional_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "transform_to_singal_with_optional" }; }

		template< typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				using ret_val_t = std::optional< so_5::transformed_message_t< Result_Msg > >;
				so_5::bind_transformer< msg_signal >(
						binding,
						from,
						[to, counter=int{0}]() mutable -> ret_val_t {
							const auto c = counter++;
							if( 1 == c )
								return std::nullopt;

							return { so_5::make_transformed< Result_Msg >( to ) };
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "t;t;", log );
			}

		template< typename Mhood >
		static void
		on_res( std::string & log, Mhood && /*cmd*/ )
			{
				log += "t";
			}
	};

template< typename Result_Msg >
void
run_test_case_for_msg_pair()
	{
		if constexpr( !std::is_same_v< msg_res_3, Result_Msg > )
			{
				run_test_case<
						so_5::single_sink_binding_t,
						Result_Msg,
						explicit_type_no_optional_t >();
				run_test_case<
						so_5::multi_sink_binding_t<>,
						Result_Msg,
						explicit_type_no_optional_t >();

				run_test_case<
						so_5::single_sink_binding_t,
						Result_Msg,
						explicit_type_with_optional_t >();
				run_test_case<
						so_5::multi_sink_binding_t<>,
						Result_Msg,
						explicit_type_with_optional_t >();
			}
		else
			{
				run_test_case<
						so_5::single_sink_binding_t,
						Result_Msg,
						transform_to_singal_no_optional_t >();
				run_test_case<
						so_5::multi_sink_binding_t<>,
						Result_Msg,
						transform_to_singal_no_optional_t >();

				run_test_case<
						so_5::single_sink_binding_t,
						Result_Msg,
						transform_to_singal_with_optional_t >();
				run_test_case<
						so_5::multi_sink_binding_t<>,
						Result_Msg,
						transform_to_singal_with_optional_t >();
			}
	}

void
run_tests()
	{
		// msg_res_1
		//
		run_test_case_for_msg_pair< msg_res_1 >();
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_res_1> >();
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_res_1> >();

		// msg_res_2
		//
		run_test_case_for_msg_pair< msg_res_2 >();
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_res_2> >();
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_res_2> >();

		// msg_res_3
		//
		run_test_case_for_msg_pair< msg_res_3 >();
	}

} /* namespace test */

int
main()
	{
		test::run_tests();

		return 0;
	}

