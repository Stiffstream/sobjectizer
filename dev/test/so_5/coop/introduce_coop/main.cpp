/*
 * Test for various variants of environment_t::introduce_coop.
 */

#include <iostream>
#include <map>
#include <exception>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
		
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
		so_default_state().event< msg_started >(
			manager_mbox( so_environment() ),
			[this] {
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
	so_5::environment_t & env,
	so_5::coop_t & coop )
{
	coop.define_agent().on_start( [&env] {
		so_5::send< msg_started >( manager_mbox( env ) );
	} );
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

#if __cplusplus < 201402
		introduce_child_coop( *this, [&env]( coop_t & coop ) {
				std::cout << "introduce_child_coop in C++11" << std::endl;
				define_agent( env, coop );
			} );
#else
		// Special check for C++14
		introduce_child_coop( *this, [&env]( auto & coop ) {
				std::cout << "introduce_child_coop in C++14" << std::endl;
				define_agent( env, coop );
			} );
#endif

		introduce_child_coop( *this,
			so_5::disp::active_obj::create_private_disp( env )->binder(),
			[&env]( coop_t & coop ) {
				define_agent( env, coop );
			} );

		introduce_child_coop( *this,
			so_5::autoname,
			[&env]( coop_t & coop ) {
				define_agent( env, coop );
			} );

		introduce_child_coop( *this,
			so_5::autoname,
			so_5::disp::active_obj::create_private_disp( env )->binder(),
			[&env]( coop_t & coop ) {
				define_agent( env, coop );
			} );

		introduce_child_coop( *this,
			"child-test-1",
			[&env]( coop_t & coop ) {
				define_agent( env, coop );
			} );

		introduce_child_coop( *this,
			"child-test-2",
			so_5::disp::one_thread::create_private_disp( env )->binder(),
			[&env]( coop_t & coop ) {
				define_agent( env, coop );
			} );
	}
};

void
init( so_5::environment_t & env )
{
	using namespace so_5;

	env.register_agent_as_coop( "main", env.make_agent< a_manager_t >( 12u ) );
	env.register_agent_as_coop( "parent", env.make_agent< a_child_owner_t >() );

#if __cplusplus < 201402
	env.introduce_coop( [&env]( coop_t & coop ) {
			std::cout << "introduce_coop in C++11" << std::endl;
			define_agent( env, coop );
		} );
#else
	env.introduce_coop( [&env]( auto & coop ) {
			std::cout << "introduce_coop in C++14" << std::endl;
			define_agent( env, coop );
		} );
#endif

	env.introduce_coop(
		so_5::disp::active_obj::create_private_disp( env )->binder(),
		[&env]( coop_t & coop ) {
			define_agent( env, coop );
		} );

	env.introduce_coop( so_5::autoname, [&env]( coop_t & coop ) {
			define_agent( env, coop );
		} );

	env.introduce_coop(
		so_5::autoname,
		so_5::disp::active_obj::create_private_disp( env )->binder(),
		[&env]( coop_t & coop ) {
			define_agent( env, coop );
		} );

	env.introduce_coop( "test-1", [&env]( coop_t & coop ) {
			define_agent( env, coop );
		} );

	env.introduce_coop( "test-2",
		so_5::disp::one_thread::create_private_disp( env )->binder(),
		[&env]( coop_t & coop ) {
			define_agent( env, coop );
		} );
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

