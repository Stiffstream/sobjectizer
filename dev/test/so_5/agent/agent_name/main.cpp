/*
 * A unit-test for agent_t::so_agent_name().
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/ensure.hpp>
#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

class anonymous_agent_t final : public so_5::agent_t
	{
	public:
		using so_5::agent_t::agent_t;

		void
		so_evt_start() override
			{
				const auto id = so_agent_name();
				std::cout << "anonymous_agent_t: " << id << std::endl;

				ensure_or_die( !id.has_actual_name(), "agent should not have a name!" );

				const auto str_id = id.to_string();
				ensure_or_die( "<noname:" == str_id.substr( 0u, 8u ),
						"unexpected prefix!" );

				so_deregister_agent_coop_normally();
			}
	};

class named_agent_t final : public so_5::agent_t
	{
	public:
		named_agent_t( context_t ctx )
			:	so_5::agent_t{ ctx + name_for_agent( "Alice" ) }
			{}

		void
		so_evt_start() override
			{
				const auto id = so_agent_name();
				std::cout << "named_agent_t: " << id << std::endl;

				ensure_or_die( id.has_actual_name(), "agent should have a name!" );

				constexpr std::string_view expected_value{ "Alice" };
				ensure_or_die( id.actual_name() == expected_value,
						"unexpected result of id.actual_name()!" );

				const auto str_id = id.to_string();
				ensure_or_die( str_id == expected_value,
						"unexpected result of id.to_string()!" );

				so_deregister_agent_coop_normally();
			}
	};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( env.make_agent< anonymous_agent_t >() );
	env.register_agent_as_coop( env.make_agent< named_agent_t >() );
}

} /* namespace test */

int
main()
{
	try
	{
		run_with_time_limit( []{ so_5::launch( test::init ); }, 5 );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}

