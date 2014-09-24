/*
 * A sample of working with dispatchers.
 */

#include <iostream>
#include <sstream>

// Main SObjectizer header files.
#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

// SObjectizer dispatchers.
#include <so_5/disp/one_thread/h/pub.hpp>
#include <so_5/disp/active_group/h/pub.hpp>
#include <so_5/disp/active_obj/h/pub.hpp>

// A class for an agent.
class a_disp_user_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:
		a_disp_user_t(
			so_5::rt::environment_t & env,
			const std::string & name )
			:
				base_type_t( env ),
				m_name( name )
		{}
		virtual ~a_disp_user_t()
		{}

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start();

		// A reaction to finish of work in SObjectizer.
		virtual void
		so_evt_finish();

	private:
		const std::string m_name;
};

void
a_disp_user_t::so_evt_start()
{
	SO_5_LOG_ERROR( so_environment(), log_stream )
	{ log_stream << m_name << ".so_evt_start(): start pause"; }

	// Sleeping for some time.
	std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

	SO_5_LOG_ERROR( so_environment(), log_stream )
	{ log_stream << m_name << ".so_evt_start(): finish pause"; }
}

void
a_disp_user_t::so_evt_finish()
{
	SO_5_LOG_ERROR( so_environment(), log_stream )
	{ log_stream << m_name << ".so_evt_finish(): start pause"; }

	// Sleeping for some time.
	std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

	SO_5_LOG_ERROR( so_environment(), log_stream )
	{ log_stream << m_name << ".so_evt_finish(): finish pause"; }
}

// The helper function for making name of an agent.
std::string
create_agent_name( const std::string & base, int i )
{
	std::ostringstream sout;
	sout << base << "_" << i;

	return sout.str();
}


// The SObjectizer Environment initialization.
void
init( so_5::rt::environment_t & env )
{
	// Creating a cooperation.
	so_5::rt::agent_coop_unique_ptr_t coop = env.create_coop( "coop" );

	// Adding agents which will work on the default dispatcher.
	for( int i = 0; i < 4; ++i )
	{
		const std::string name = create_agent_name( "default_disp", i+1 );

		coop->add_agent( new a_disp_user_t( env, name ) );
	}

	// Adding agents which will work on the dispatcher 
	// with name 'single_thread'.
	for( int i = 0; i < 3; ++i )
	{
		const std::string name = create_agent_name( "single_thread", i+1 );

		coop->add_agent(
			new a_disp_user_t( env, name ),
			so_5::disp::one_thread::create_disp_binder(
				"single_thread" ) );
	}

	// Adding agents which will work on the dispatcher with active groups
	// named as 'active_group'. Agents will be bound to a group 'A'.
	for( int i = 0; i < 2; ++i )
	{
		const std::string name = create_agent_name( "active_group_A", i+1 );

		coop->add_agent(
			new a_disp_user_t( env, name ),
			so_5::disp::active_group::create_disp_binder(
				"active_group",
				"A" ) );
	}

	// Adding agents which will work on the dispatcher with active groups
	// named as 'active_group'. Agents will be bound to a group 'B'.
	for( int i = 0; i < 2; ++i )
	{
		const std::string name = create_agent_name( "active_group_B", i+1 );

		coop->add_agent(
			new a_disp_user_t( env, name ),
			so_5::disp::active_group::create_disp_binder(
				"active_group",
				"B" ) );
	}

	// Adding agents which will work on the dispatcher for active objects.
	// This dispatcher will have name 'active_obj'.
	for( int i = 0; i < 4; ++i )
	{
		const std::string name = create_agent_name( "active_obj", i+1 );

		coop->add_agent(
			new a_disp_user_t( env, name ),
			so_5::disp::active_obj::create_disp_binder(
				"active_obj" ) );
	}

	// Registering the cooperation.
	env.register_coop( std::move( coop ) );

	// Stopping SObjectizer.
	env.stop();
}

int
main( int, char ** argv )
{
	try
	{
		so_5::api::run_so_environment(
			&init,
			[]( so_5::rt::environment_params_t & p )
			{
				p.add_named_dispatcher(
					"single_thread",
					so_5::disp::one_thread::create_disp() )
				.add_named_dispatcher(
					"active_group",
					so_5::disp::active_group::create_disp() )
				.add_named_dispatcher(
					"active_obj",
					so_5::disp::active_obj::create_disp() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

