/*
 * Test cases for bind_then_transform and messages.
 */

#include "common.hpp"

namespace test {

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

#if 0
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
#endif

void
run_implicit_type_no_optional_no_dr();

void
run_implicit_type_no_optional_with_dr();

void
run_implicit_type_with_optional_no_dr();

void
run_implicit_type_with_optional_with_dr();

void
run_explicit_type_no_optional_no_dr();

void
run_explicit_type_no_optional_with_dr();

void
run_explicit_type_with_optional_no_dr();

void
run_explicit_type_with_optional_with_dr();

void
run_tests()
	{
		run_implicit_type_no_optional_no_dr();
		run_implicit_type_no_optional_with_dr();
		run_implicit_type_with_optional_no_dr();
		run_implicit_type_with_optional_with_dr();
		run_explicit_type_no_optional_no_dr();
		run_explicit_type_no_optional_with_dr();
		run_explicit_type_with_optional_no_dr();
		run_explicit_type_with_optional_with_dr();
	}

} /* namespace test */

int
main()
	{
		test::run_tests();

		return 0;
	}

