/*
 * Test for various variants of environment_t::introduce_coop.
 */

#include <iostream>
#include <map>
#include <exception>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

using namespace std::string_literals;
		
so_5::mbox_t
manager_mbox( so_5::environment_t & env )
{
	return env.create_mbox( "manager" );
}

struct msg_started : public so_5::signal_t {};

class a_manager_t : public so_5::agent_t
{
public :
	a_manager_t(
		context_t ctx,
		unsigned int expected )
		:	so_5::agent_t( ctx )
		,	m_expected( expected )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state().event(
			manager_mbox( so_environment() ),
			[this](mhood_t< msg_started >) {
				++m_received;
				if( m_received == m_expected )
					so_environment().stop();
			} );
	}

private :
	const unsigned int m_expected;
	unsigned int m_received = { 0 };
};

void
define_agent(
	so_5::coop_t & coop )
{
	class a_starter_t final : public so_5::agent_t
	{
	public:
		using so_5::agent_t::agent_t;

		void so_evt_start() override
		{
			so_5::send< msg_started >( manager_mbox( so_environment() ) );
		}
	};

	coop.make_agent< a_starter_t >();
}

void
ensure_valid_value(
	const std::string & actual,
	const std::string & expected )
{
	ensure_or_die( actual == expected,
			"values mismatch! actual='" + actual
			+ "', expected='" + expected + "'" );
}

class a_child_owner_t : public so_5::agent_t
{
public :
	a_child_owner_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_evt_start() override
	{
		using namespace so_5;

		auto & env = so_environment();

		// Special check for C++14
		ensure_valid_value(
			introduce_child_coop( *this, []( auto & coop ) {
					std::cout << "introduce_child_coop in C++14 or later." << std::endl;
					define_agent( coop );
					return "first"s;
				} ),
			"first"s );

		ensure_valid_value(
			introduce_child_coop( *this,
				so_5::disp::active_obj::make_dispatcher( env ).binder(),
				[]( coop_t & coop ) {
					define_agent( coop );
					return "second"s;
				} ),
			"second"s );

		ensure_valid_value(
			introduce_child_coop( *this,
				so_5::autoname,
				[]( coop_t & coop ) {
					define_agent( coop );
					return "third"s;
				} ),
			"third"s );

		ensure_valid_value(
			introduce_child_coop( *this,
				so_5::autoname,
				so_5::disp::active_obj::make_dispatcher( env ).binder(),
				[]( coop_t & coop ) {
					define_agent( coop );
					return "fourth"s;
				} ),
			"fourth"s );

		ensure_valid_value(
			introduce_child_coop( *this,
				"child-test-1",
				[]( coop_t & coop ) {
					define_agent( coop );
					return "fifth"s;
				} ),
			"fifth"s );

		ensure_valid_value(
			introduce_child_coop( *this,
				"child-test-2",
				so_5::disp::one_thread::make_dispatcher( env ).binder(),
				[]( coop_t & coop ) {
					define_agent( coop );
					return "sixth"s;
				} ),
			"sixth"s );
	}
};

void
init( so_5::environment_t & env )
{
	using namespace so_5;

	env.register_agent_as_coop( "main", env.make_agent< a_manager_t >( 12u ) );
	env.register_agent_as_coop( "parent", env.make_agent< a_child_owner_t >() );

	ensure_valid_value(
		env.introduce_coop( []( auto & coop ) {
				std::cout << "introduce_coop in C++14 or later." << std::endl;
				define_agent( coop );
				return "first"s;
			} ),
		"first"s );

	ensure_valid_value(
		env.introduce_coop(
			so_5::disp::active_obj::make_dispatcher( env ).binder(),
			[]( coop_t & coop ) {
				define_agent( coop );
				return "second"s;
			} ),
		"second"s );

	ensure_valid_value(
		env.introduce_coop( so_5::autoname, []( coop_t & coop ) {
				define_agent( coop );
				return "third"s;
			} ),
		"third"s );

	ensure_valid_value(
		env.introduce_coop(
			so_5::autoname,
			so_5::disp::active_obj::make_dispatcher( env ).binder(),
			[]( coop_t & coop ) {
				define_agent( coop );
				return "fourth"s;
			} ),
		"fourth"s );

	ensure_valid_value(
		env.introduce_coop( "test-1", []( coop_t & coop ) {
				define_agent( coop );
				return "fifth"s;
			} ),
		"fifth"s );

	ensure_valid_value(
		env.introduce_coop( "test-2",
			so_5::disp::one_thread::make_dispatcher( env ).binder(),
			[]( coop_t & coop ) {
				define_agent( coop );
				return "sixth"s;
			} ),
		"sixth"s );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			20,
			"introduce_coop test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

