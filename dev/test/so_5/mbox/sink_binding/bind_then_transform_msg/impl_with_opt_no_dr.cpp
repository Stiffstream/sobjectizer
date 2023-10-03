#include "common.hpp"

namespace test {

struct implicit_type_with_optional_no_dr_t : public implicit_type_case_t
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

void
run_implicit_type_with_optional_no_dr()
	{
		run_tests_for_case_handler< implicit_type_with_optional_no_dr_t >();
	}

} /* namespace test */


