/*
 * A sample of usage of agent name.
 */

#include <so_5/all.hpp>

#include <iostream>
#include <set>

// Name of mbox for sending information to agents registry.
const std::string registry_mbox_name{ "agents_registry" };

// Agent that plays role of agents registry.
class agents_registry final : public so_5::agent_t
{
public:
	// Message for informing about start of an agent.
	struct agent_started final : public so_5::message_t
	{
		const std::string m_name;

		agent_started( std::string name ) : m_name{ std::move(name) }
		{}
	};

	// Message for informing that agent finished its work.
	struct agent_finished final : public so_5::message_t
	{
		const std::string m_name;

		agent_finished( std::string name ) : m_name{ std::move(name) }
		{}
	};

	agents_registry( context_t ctx )
		:	so_5::agent_t{ std::move(ctx) }
	{}

	void so_define_agent() override
	{
		so_subscribe( so_environment().create_mbox( registry_mbox_name ) )
			.event( &agents_registry::evt_agent_started )
			.event( &agents_registry::evt_agent_finished )
			;

		so_subscribe_self()
			.event( &agents_registry::evt_show_registry )
			;
	}

	void so_evt_start() override
	{
		// Initiate periodic message for showing the content of the registry.
		m_show_timer = so_5::send_periodic< show_registry >( *this,
				std::chrono::milliseconds{ 20 },
				std::chrono::milliseconds{ 20 } );
	}

private:
	// Periodic signal for showing the current content of the registry.
	struct show_registry final : public so_5::signal_t {};

	// A list of currently working agents.
	std::set< std::string > m_registry;

	// Timer for showing the current content of the registry.
	so_5::timer_id_t m_show_timer;

	void evt_agent_started( mhood_t<agent_started> cmd )
	{
		m_registry.insert( cmd->m_name );

		std::cout << " started: " << cmd->m_name << std::endl;
	}

	void evt_agent_finished( mhood_t<agent_finished> cmd )
	{
		m_registry.erase( cmd->m_name );

		std::cout << "finished: " << cmd->m_name << std::endl;

		if( m_registry.empty() )
			// It's time to complete.
			so_deregister_agent_coop_normally();
	}

	void evt_show_registry( mhood_t<show_registry> )
	{
		std::cout << "--- registry begin ---" << std::endl;
		for( const auto & name : m_registry )
			std::cout << "   " << name << std::endl;
		std::cout << "--- registry end ---" << std::endl;
	}
};

// Base class for worker. Agents of this type have no names.
class simple_worker : public so_5::agent_t
{
public:
	simple_worker( context_t ctx, std::chrono::milliseconds work_time )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_work_time{ work_time }
	{}

	void so_define_agent() override
	{
		so_subscribe_self().event( &simple_worker::evt_done );
	}

	void so_evt_start() override
	{
		// Should inform about the start.
		so_5::send< agents_registry::agent_started >(
				so_environment().create_mbox( registry_mbox_name ),
				so_agent_name().to_string() );

		// Limit the work time.
		so_5::send_delayed< done >( *this, m_work_time );
	}

	void so_evt_finish() override
	{
		// Should inform about the finish.
		so_5::send< agents_registry::agent_finished >(
				so_environment().create_mbox( registry_mbox_name ),
				so_agent_name().to_string() );
	}

private:
	// Signal for limiting work time of the agent.
	struct done final : public so_5::signal_t {};

	// How long the agent should work.
	const std::chrono::milliseconds m_work_time;

	void evt_done( mhood_t<done> )
	{
		so_deregister_agent_coop_normally();
	}
};

// Type of worker with name.
//
// It's derived from simple_worker just to make example code shorter.
class named_worker : public simple_worker
{
public:
	named_worker( context_t ctx, std::string_view name, std::chrono::milliseconds work_time )
		:	simple_worker{ ctx + name_for_agent(name), work_time }
	{}

	// There is no need to define something else.
};

int main()
{
	try
	{
		// Starting SObjectizer.
		so_5::launch(
			// A function for SO Environment initialization.
			[]( so_5::environment_t & env )
			{
				// Starting the registry.
				env.register_agent_as_coop(
						env.make_agent< agents_registry >() );

				// Starting workers.
				using namespace std::chrono_literals;
				env.register_agent_as_coop(
						env.make_agent< simple_worker >( 30ms ) );
				env.register_agent_as_coop(
						env.make_agent< simple_worker >( 50ms ) );
				env.register_agent_as_coop(
						env.make_agent< named_worker >( "Alice", 50ms ) );
				env.register_agent_as_coop(
						env.make_agent< simple_worker >( 70ms ) );
				env.register_agent_as_coop(
						env.make_agent< named_worker >( "Bob", 90ms ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

