#include "common.hpp"

namespace test {

struct explicit_type_no_optional_no_dr_t : public explicit_type_case_t
	{
		[[nodiscard]] static std::string_view
		name() { return { "explicit_type_no_optional_no_dr" }; }

		template< typename Source_Msg, typename Result_Msg, typename Binding >
		static void
		tune_binding( Binding & binding, const so_5::mbox_t & from, const so_5::mbox_t & to )
			{
				so_5::bind_transformer< Source_Msg >(
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

void
run_explicit_type_no_optional_no_dr()
	{
		run_tests_for_case_handler< explicit_type_no_optional_no_dr_t >();
	}

} /* namespace test */

