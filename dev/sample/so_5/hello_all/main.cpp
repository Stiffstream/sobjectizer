/*
 * A sample of the message interchange.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

struct msg_hello_to_all
{
	// Sender name.
	const std::string m_sender;
	// Sender Mbox.
	const so_5::mbox_t m_mbox;
};

struct msg_hello_to_you
{
	// Sender name.
	const std::string m_sender;
};

// An agent class.
class a_hello_t : public so_5::agent_t
{
	public:
		a_hello_t( context_t ctx, std::string agent_name )
			:	so_5::agent_t( ctx )
			,	m_agent_name( std::move( agent_name ) )
			,	m_common_mbox( so_environment().create_mbox( "common_mbox" ) )
		{}

		// Definition of the agent for SObjectizer.
		virtual void so_define_agent() override;

		// A reaction to start of work in SObjectizer.
		virtual void so_evt_start() override;

		void evt_hello_to_all(
			const msg_hello_to_all & evt_data );

		void evt_hello_to_you(
			const msg_hello_to_you & evt_data );

	private:
		// Agent name.
		const std::string m_agent_name;

		// Common mbox for all sample agents.
		const so_5::mbox_t m_common_mbox;
};

void a_hello_t::so_define_agent()
{
	// Message subscription.
	so_subscribe( m_common_mbox )
		.event( &a_hello_t::evt_hello_to_all );

	so_subscribe_self()
		.event( &a_hello_t::evt_hello_to_you );
}

void a_hello_t::so_evt_start()
{
	std::cout << m_agent_name << ".so_evt_start" << std::endl;

	// Send greeting to all agents.
	so_5::send< msg_hello_to_all >(
		m_common_mbox,
		m_agent_name, so_direct_mbox() );
}

void a_hello_t::evt_hello_to_all(
	const msg_hello_to_all & evt_data )
{
	std::cout << m_agent_name << ".evt_hello_to_all: "
		<< evt_data.m_sender << std::endl;

	// If this agent is not the sender then reply should be sent.
	if( m_agent_name != evt_data.m_sender )
	{
		so_5::send< msg_hello_to_you >( evt_data.m_mbox, m_agent_name );
	}
}

void a_hello_t::evt_hello_to_you(
	const msg_hello_to_you & evt_data )
{
	std::cout << m_agent_name << ".evt_hello_to_you: "
		<< evt_data.m_sender << std::endl;
}

// The SObjectizer Environment initialization.
void init( so_5::environment_t & env )
{
	// Creating and registering a cooperation.
	env.introduce_coop( []( so_5::coop_t & coop ) {
		// Adding agents to the cooperation.
		coop.make_agent< a_hello_t >( "alpha" );
		coop.make_agent< a_hello_t >( "beta" );
		coop.make_agent< a_hello_t >( "gamma" );
	} );

	// Give some time to agents.
	std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
	env.stop();
}

int main()
{
	try
	{
		so_5::launch( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
