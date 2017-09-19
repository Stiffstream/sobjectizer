/*
 * A sample of working with dispatchers.
 */

#include <iostream>
#include <sstream>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// A class for an agent.
class a_disp_user_t : public so_5::agent_t
{
	public:
		a_disp_user_t( context_t ctx, std::string name )
			: so_5::agent_t( ctx )
			, m_name( std::move(name) )
		{}

		// A reaction to start of work in SObjectizer.
		virtual void so_evt_start() override
		{
			SO_5_LOG_ERROR( so_environment(), log_stream )
			{ log_stream << m_name << ".so_evt_start(): start pause"; }

			// Sleeping for some time.
			std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

			SO_5_LOG_ERROR( so_environment(), log_stream )
			{ log_stream << m_name << ".so_evt_start(): finish pause"; }
		}

		// A reaction to finish of work in SObjectizer.
		virtual void so_evt_finish() override
		{
			SO_5_LOG_ERROR( so_environment(), log_stream )
			{ log_stream << m_name << ".so_evt_finish(): start pause"; }

			// Sleeping for some time.
			std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

			SO_5_LOG_ERROR( so_environment(), log_stream )
			{ log_stream << m_name << ".so_evt_finish(): finish pause"; }
		}

	private:
		const std::string m_name;
};

// The helper function for making name of an agent.
std::string create_agent_name( const std::string & base, int i )
{
	return base + "_" + std::to_string(i);
}


// The SObjectizer Environment initialization.
void init( so_5::environment_t & env )
{
	// Creating and registering a cooperation.
	env.introduce_coop( []( so_5::coop_t & coop ) {
		// Adding agents which will work on the default dispatcher.
		for( int i = 0; i < 4; ++i )
		{
			coop.make_agent< a_disp_user_t >(
				create_agent_name( "default_disp", i+1 ) );
		}

		// Adding agents which will work on the dispatcher 
		// with name 'single_thread'.
		for( int i = 0; i < 3; ++i )
		{
			coop.make_agent_with_binder< a_disp_user_t >(
				so_5::disp::one_thread::create_disp_binder( "single_thread" ),
				create_agent_name( "single_thread", i+1 ) );
		}

		// Adding agents which will work on the dispatcher with active groups
		// named as 'active_group'. Agents will be bound to a group 'A'.
		for( int i = 0; i < 2; ++i )
		{
			coop.make_agent_with_binder< a_disp_user_t >(
				so_5::disp::active_group::create_disp_binder( "active_group", "A" ),
				create_agent_name( "active_group_A", i+1 ) );
		}

		// Adding agents which will work on the dispatcher with active groups
		// named as 'active_group'. Agents will be bound to a group 'B'.
		for( int i = 0; i < 2; ++i )
		{
			coop.make_agent_with_binder< a_disp_user_t >(
				so_5::disp::active_group::create_disp_binder( "active_group", "B" ),
				create_agent_name( "active_group_B", i+1 ) );
		}

		// Adding agents which will work on the dispatcher for active objects.
		// This dispatcher will have name 'active_obj'.
		for( int i = 0; i < 4; ++i )
		{
			coop.make_agent_with_binder< a_disp_user_t >(
				so_5::disp::active_obj::create_disp_binder( "active_obj" ),
				create_agent_name( "active_obj", i+1 ) );
		}
	});

	// Stopping SObjectizer.
	env.stop();
}

int main()
{
	try
	{
		so_5::launch(
			&init,
			[]( so_5::environment_params_t & p )
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

