/*
 * Test cases for bind_then_transform and messages.
 */

#include <iostream>
#include <sstream>

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

template< typename Msg >
struct is_implicit_case_applicable_impl_t
	{
		static constexpr bool value = true;
	};

template< typename Msg >
struct is_implicit_case_applicable_impl_t< so_5::immutable_msg<Msg> >
	{
		static constexpr bool value = false;
	};

template< typename Msg >
struct is_implicit_case_applicable_impl_t< so_5::mutable_msg<Msg> >
	{
		static constexpr bool value = false;
	};

template< typename Msg >
constexpr bool is_implicit_case_applicable_v =
		is_implicit_case_applicable_impl_t<Msg>::value;

struct implicit_type_no_optional_no_dr_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "implicit_type_no_optional_no_dr" }; }

		template< typename Source_Msg, typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				so_5::bind_then_transform(
						binding,
						from,
						[to]( const Source_Msg & src ) {
							return so_5::make_transformed< Result_Msg >( to,
									std::to_string( src.m_a ) + "-" + std::to_string( src.m_c ) );
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "1-3;2-4;3-5;4-6;", log );
			}
	};

struct implicit_type_no_optional_with_dr_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "implicit_type_no_optional_with_dr" }; }

		template< typename Source_Msg, typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				so_5::bind_then_transform(
						binding,
						from,
						[to]( const Source_Msg & src ) {
							return so_5::make_transformed< Result_Msg >( to,
									std::to_string( src.m_a ) + "-" + std::to_string( src.m_c ) );
						},
						[]( dr_param_from_source_msg_t<Source_Msg> const & src ) {
							return 1 != src.m_a;
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "2-4;3-5;4-6;", log );
			}
	};

struct implicit_type_with_optional_no_dr_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "implicit_type_with_optional_no_dr" }; }

		template< typename Source_Msg, typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				using ret_val_t = std::optional< so_5::transformed_message_t< Result_Msg > >;
				so_5::bind_then_transform(
						binding,
						from,
						[to]( const Source_Msg & src ) -> ret_val_t {
							if( 3 == src.m_a && 4 == src.m_b && 5 == src.m_c )
								return std::nullopt;

							return { so_5::make_transformed< Result_Msg >( to,
									std::to_string( src.m_a ) + "-" + std::to_string( src.m_c ) ) };
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "1-3;2-4;4-6;", log );
			}
	};

struct implicit_type_with_optional_with_dr_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "implicit_type_with_optional_with_dr" }; }

		template< typename Source_Msg, typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				using ret_val_t = std::optional< so_5::transformed_message_t< Result_Msg > >;
				so_5::bind_then_transform(
						binding,
						from,
						[to]( const Source_Msg & src ) -> ret_val_t {
							if( 3 == src.m_a && 4 == src.m_b && 5 == src.m_c )
								return std::nullopt;

							return { so_5::make_transformed< Result_Msg >( to,
									std::to_string( src.m_a ) + "-" + std::to_string( src.m_c ) ) };
						},
						[]( dr_param_from_source_msg_t<Source_Msg> const & src ) {
							return 1 != src.m_a;
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "2-4;4-6;", log );
			}
	};

struct explicit_type_no_optional_no_dr_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "explicit_type_no_optional_no_dr" }; }

		template< typename Source_Msg, typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				so_5::bind_then_transform< Source_Msg >(
						binding,
						from,
						[to]( auto && src ) {
							return so_5::make_transformed< Result_Msg >( to,
									std::to_string( src.m_a ) + "-" + std::to_string( src.m_c ) );
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "1-3;2-4;3-5;4-6;", log );
			}
	};

struct explicit_type_no_optional_with_dr_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "explicit_type_no_optional_with_dr" }; }

		template< typename Source_Msg, typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				so_5::bind_then_transform< Source_Msg >(
						binding,
						from,
						[to]( auto && src ) {
							return so_5::make_transformed< Result_Msg >( to,
									std::to_string( src.m_a ) + "-" + std::to_string( src.m_c ) );
						},
						[]( dr_param_from_source_msg_t<Source_Msg> const & src ) {
							return 1 != src.m_a;
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "2-4;3-5;4-6;", log );
			}
	};

struct explicit_type_with_optional_no_dr_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "explicit_type_with_optional_no_dr" }; }

		template< typename Source_Msg, typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				using ret_val_t = std::optional< so_5::transformed_message_t< Result_Msg > >;
				so_5::bind_then_transform< Source_Msg >(
						binding,
						from,
						[to]( auto && src ) -> ret_val_t {
							if( 3 == src.m_a && 4 == src.m_b && 5 == src.m_c )
								return std::nullopt;

							return { so_5::make_transformed< Result_Msg >( to,
									std::to_string( src.m_a ) + "-" + std::to_string( src.m_c ) ) };
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "1-3;2-4;4-6;", log );
			}
	};

struct explicit_type_with_optional_with_dr_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "explicit_type_with_optional_with_dr" }; }

		template< typename Source_Msg, typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				using ret_val_t = std::optional< so_5::transformed_message_t< Result_Msg > >;
				so_5::bind_then_transform< Source_Msg >(
						binding,
						from,
						[to]( auto && src ) -> ret_val_t {
							if( 3 == src.m_a && 4 == src.m_b && 5 == src.m_c )
								return std::nullopt;

							return { so_5::make_transformed< Result_Msg >( to,
									std::to_string( src.m_a ) + "-" + std::to_string( src.m_c ) ) };
						},
						[]( dr_param_from_source_msg_t<Source_Msg> const & src ) {
							return 1 != src.m_a;
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "2-4;4-6;", log );
			}
	};

template< typename Source_Msg, typename Result_Msg >
void
run_test_case_for_msg_pair()
	{
		if constexpr( is_implicit_case_applicable_v< Source_Msg > )
			{
				run_test_case<
						so_5::single_sink_binding_t,
						Source_Msg, Result_Msg,
						implicit_type_no_optional_no_dr_t >();
				run_test_case<
						so_5::multi_sink_binding_t<>,
						Source_Msg, Result_Msg,
						implicit_type_no_optional_no_dr_t >();
				run_test_case<
						so_5::single_sink_binding_t,
						Source_Msg, Result_Msg,
						implicit_type_no_optional_with_dr_t >();
				run_test_case<
						so_5::multi_sink_binding_t<>,
						Source_Msg, Result_Msg,
						implicit_type_no_optional_with_dr_t >();
				run_test_case<
						so_5::single_sink_binding_t,
						Source_Msg, Result_Msg,
						implicit_type_with_optional_no_dr_t >();
				run_test_case<
						so_5::multi_sink_binding_t<>,
						Source_Msg, Result_Msg,
						implicit_type_with_optional_no_dr_t >();
				run_test_case<
						so_5::single_sink_binding_t,
						Source_Msg, Result_Msg,
						implicit_type_with_optional_with_dr_t >();
				run_test_case<
						so_5::multi_sink_binding_t<>,
						Source_Msg, Result_Msg,
						implicit_type_with_optional_with_dr_t >();
			}

		run_test_case<
				so_5::single_sink_binding_t,
				Source_Msg, Result_Msg,
				explicit_type_no_optional_no_dr_t >();
		run_test_case<
				so_5::multi_sink_binding_t<>,
				Source_Msg, Result_Msg,
				explicit_type_no_optional_no_dr_t >();
		run_test_case<
				so_5::single_sink_binding_t,
				Source_Msg, Result_Msg,
				explicit_type_no_optional_with_dr_t >();
		run_test_case<
				so_5::multi_sink_binding_t<>,
				Source_Msg, Result_Msg,
				explicit_type_no_optional_with_dr_t >();
		run_test_case<
				so_5::single_sink_binding_t,
				Source_Msg, Result_Msg,
				explicit_type_with_optional_no_dr_t >();
		run_test_case<
				so_5::multi_sink_binding_t<>,
				Source_Msg, Result_Msg,
				explicit_type_with_optional_no_dr_t >();
		run_test_case<
				so_5::single_sink_binding_t,
				Source_Msg, Result_Msg,
				explicit_type_with_optional_with_dr_t >();
		run_test_case<
				so_5::multi_sink_binding_t<>,
				Source_Msg, Result_Msg,
				explicit_type_with_optional_with_dr_t >();
	}

void
run_tests()
	{
		// msg_src_1, msg_res_1
		//
		run_test_case_for_msg_pair< msg_src_1, msg_res_1 >();

		// msg_src_2, msg_res_1
		//
		run_test_case_for_msg_pair< msg_src_2, msg_res_1 >();

		// msg_src_2, msg_res_2
		//
		run_test_case_for_msg_pair< msg_src_2, msg_res_2 >();

		// msg_src_1, msg_res_2
		//
		run_test_case_for_msg_pair< msg_src_1, msg_res_2 >();

		// so_5::immutable_msg.

		// msg_src_1, msg_res_1
		//
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_1>, msg_res_1 >();
		run_test_case_for_msg_pair< msg_src_1, so_5::immutable_msg<msg_res_1> >();
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_1>, so_5::immutable_msg<msg_res_1> >();

		// msg_src_2, msg_res_1
		//
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_2>, msg_res_1 >();
		run_test_case_for_msg_pair< msg_src_2, so_5::immutable_msg<msg_res_1> >();
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_2>, so_5::immutable_msg<msg_res_1> >();

		// msg_src_2, msg_res_2
		//
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_2>, msg_res_2 >();
		run_test_case_for_msg_pair< msg_src_2, so_5::immutable_msg<msg_res_2> >();
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_2>, so_5::immutable_msg<msg_res_2> >();

		// msg_src_1, msg_res_2
		//
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_1>, msg_res_2 >();
		run_test_case_for_msg_pair< msg_src_1, so_5::immutable_msg<msg_res_2> >();
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_1>, so_5::immutable_msg<msg_res_2> >();

		// so_5::mutable_msg.

		// msg_src_1, msg_res_1
		//
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_1>, msg_res_1 >();
		run_test_case_for_msg_pair< msg_src_1, so_5::mutable_msg<msg_res_1> >();
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_1>, so_5::mutable_msg<msg_res_1> >();

		// msg_src_2, msg_res_1
		//
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_2>, msg_res_1 >();
		run_test_case_for_msg_pair< msg_src_2, so_5::mutable_msg<msg_res_1> >();
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_2>, so_5::mutable_msg<msg_res_1> >();

		// msg_src_2, msg_res_2
		//
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_2>, msg_res_2 >();
		run_test_case_for_msg_pair< msg_src_2, so_5::mutable_msg<msg_res_2> >();
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_2>, so_5::mutable_msg<msg_res_2> >();

		// msg_src_1, msg_res_2
		//
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_1>, msg_res_2 >();
		run_test_case_for_msg_pair< msg_src_1, so_5::mutable_msg<msg_res_2> >();
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_1>, so_5::mutable_msg<msg_res_2> >();
		// so_5::mutable_msg + so_5::immutable_msg.

		// msg_src_1, msg_res_1
		//
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_1>, so_5::immutable_msg<msg_res_1> >();
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_1>, so_5::mutable_msg<msg_res_1> >();

		// msg_src_2, msg_res_1
		//
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_2>, so_5::immutable_msg<msg_res_1> >();
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_2>, so_5::mutable_msg<msg_res_1> >();

		// msg_src_2, msg_res_2
		//
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_2>, so_5::immutable_msg<msg_res_2> >();
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_2>, so_5::mutable_msg<msg_res_2> >();

		// msg_src_1, msg_res_2
		//
		run_test_case_for_msg_pair< so_5::mutable_msg<msg_src_1>, so_5::immutable_msg<msg_res_2> >();
		run_test_case_for_msg_pair< so_5::immutable_msg<msg_src_1>, so_5::mutable_msg<msg_res_2> >();
	}

} /* namespace test */

int
main()
	{
		test::run_tests();

		return 0;
	}

