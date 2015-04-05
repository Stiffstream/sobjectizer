/*
 * Demonstration of usage of the same private dispatcher for children
 * cooperations.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

// A hello message to be sent to coordinator.
struct msg_hello : public so_5::rt::message_t
{
	std::string m_text;

	msg_hello( std::string text ) : m_text( std::move( text ) )
	{}
};

// Child agent.
// Sends a hello message to the coordinator at start.
// Then if creation of another child cooperation is enabled creates
// child cooperation.
class a_child_t : public so_5::rt::agent_t
{
public :
	a_child_t(
		so_5::rt::environment_t & env,
		// Mbox of the coordinator.
		so_5::rt::mbox_t coordinator,
		// Dispatcher to be used for child.
		so_5::disp::active_obj::private_dispatcher_handle_t dispatcher,
		// Current generation.
		int generation,
		// Max generation.
		int max_generation )
		:	so_5::rt::agent_t( env )
		,	m_coordinator( std::move( coordinator ) )
		,	m_dispatcher( std::move( dispatcher ) )
		,	m_generation( generation )
		,	m_max_generation( max_generation )
	{}

	virtual void
	so_evt_start()
	{
		send_hello_to_coordinator();

		if( m_generation < m_max_generation )
			create_child_coop();
	}

private :
	const so_5::rt::mbox_t m_coordinator;

	so_5::disp::active_obj::private_dispatcher_handle_t m_dispatcher;

	const int m_generation;
	const int m_max_generation;
	
	void
	send_hello_to_coordinator()
	{
		// Hello message must contain agent name and thread id.
		std::ostringstream ss;
		ss << "child at generation " << m_generation
			<< " on thread: "
			<< std::this_thread::get_id();

		// Coordinator should receive a hello message.
		so_5::send< msg_hello >( m_coordinator, ss.str() );
	}

	void
	create_child_coop()
	{
		auto coop = so_5::rt::create_child_coop(
				*this,
				// Name will be generated automatically.
				so_5::autoname,
				// The same dispatcher will be used for child cooperation.
				m_dispatcher->binder() );

		coop->make_agent< a_child_t >(
				m_coordinator,
				m_dispatcher,
				m_generation + 1,
				m_max_generation );

		so_environment().register_coop( std::move( coop ) );
	}
};

// Sample coordinator.
// Works on the default dispatcher.
// Receives messages from children agents.
// Finishes sample when all messages are received.
class a_coordinator_t : public so_5::rt::agent_t
{
public :
	a_coordinator_t( so_5::rt::environment_t & env )
		:	so_5::rt::agent_t( env )
	{}

	virtual void
	so_define_agent() override
	{
		// Just one message must be handled in the default agent state.
		so_default_state().event( [this]( const msg_hello & msg ) {
				std::cout << "hello: " << msg.m_text << std::endl;

				// Work must be stopped if all messages are received.
				if( 0 == (--m_remaining_messages) )
					so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		// Cooperation must be created at the start of agent's work.
		create_first_child_coop();
	}

private :
	unsigned int m_remaining_messages = 6;

	void
	create_first_child_coop()
	{
		// The private dispatcher for the family of child cooperations.
		auto disp = so_5::disp::active_obj::create_private_disp(
				so_environment() );
		auto coop = so_5::rt::create_child_coop(
				// This agent will be parent for new cooperation.
				*this,
				// Name for the cooperation will be generated automatically.
				so_5::autoname,
				// The main dispatcher for the new cooperation is
				// the private dispatcher.
				disp->binder() );

		coop->make_agent< a_child_t >( so_direct_mbox(), disp, 1, 6 );

		so_environment().register_coop( std::move( coop ) );
	}
};

int
main()
{
	try
	{
		so_5::launch(
			[]( so_5::rt::environment_t & env ) {
				// Coordinator agent will work on the default dispatcher.
				env.register_agent_as_coop(
						so_5::autoname,
						env.make_agent< a_coordinator_t >() );
			} );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

