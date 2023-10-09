#include "common.hpp"

namespace test {

struct implicit_type_no_optional_with_dr2_t : public implicit_type_case_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "implicit_type_no_optional_with_dr2" }; }

		template< typename Source_Msg, typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				so_5::bind_transformer(
						binding,
						from,
						[to]( const Source_Msg & src ) {
							return so_5::make_transformed< Result_Msg >( to,
									std::to_string( src.m_a ) + "-" + std::to_string( src.m_c ) );
						},
						[]( const auto & src ) {
							return 1 != src.m_a;
						} );
			}

		static void
		check_result( std::string_view log )
			{
				ensure_valid_or_die( name(), "2-4;3-5;4-6;", log );
			}
	};

void
run_implicit_type_no_optional_with_dr2()
	{
		run_tests_for_case_handler< implicit_type_no_optional_with_dr2_t >();
	}

} /* namespace test */


