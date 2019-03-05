/*
 * Demonstration of very simple case for dispatchers.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

// Sample coordinator.
// Works on the default dispatcher.
// Receives messages from children agents.
// Finishes sample when all messages are received.
class a_coordinator_t : public so_5::agent_t
{
public :
	a_coordinator_t( context_t ctx ) : so_5::agent_t( ctx )
	{}

	virtual void so_define_agent() override
	{
		// Just one message must be handled in the default agent state.
		so_default_state().event( [this]( const std::string & msg ) {
				std::cout << "hello: " << msg << std::endl;

				// Work must be stopped if all messages are received.
				if( 0 == (--m_remaining_messages) )
					so_deregister_agent_coop_normally();
			} );
	}

	virtual void so_evt_start() override
	{
		// Cooperation must be created at the start of agent's work.

		create_first_child_coop();
		create_second_child_coop();
	}

private :
	unsigned int m_remaining_messages = 6;

	void create_first_child_coop()
	{
		// The first cooperation will use one_thread dispatcher.
		auto disp = so_5::disp::one_thread::make_dispatcher(
				so_environment() );
		auto coop = so_5::create_child_coop(
				// This agent will be parent for new cooperation.
				*this,
				// Name for the cooperation will be generated automatically.
				so_5::autoname,
				// The main dispatcher for the new cooperation is
				// one_thread dispatcher.
				disp.binder() );

		fill_and_register_coop( std::move( coop ), "one_thread" );
	}

	void create_second_child_coop()
	{
		// The second cooperation will use active_obj dispatcher.
		auto disp = so_5::disp::active_obj::make_dispatcher(
				so_environment() );
		auto coop = so_5::create_child_coop(
				// This agent will be parent for new cooperation.
				*this,
				// Name for the cooperation will be generated automatically.
				so_5::autoname,
				// The main dispatcher for the new cooperation is
				// active_obj dispatcher.
				disp.binder() );

		fill_and_register_coop( std::move( coop ), "active_obj" );
	}

	void fill_and_register_coop(
		so_5::coop_unique_ptr_t coop,
		const std::string & agent_name_prefix )
	{
		// Type of agent to be used in child coop.
		class demo_agent_t final : public so_5::agent_t {
			const so_5::mbox_t m_mbox;
			const std::string m_agent_name;
		public :
			demo_agent_t(
				context_t ctx,
				so_5::mbox_t mbox,
				std::string agent_name )
				:	so_5::agent_t{ std::move(ctx) }
				,	m_mbox{ std::move(mbox) }
				,	m_agent_name{ std::move(agent_name) }
			{}

			void so_evt_start() override
			{
				// Hello message must contain agent name and thread id.
				std::ostringstream ss;
				ss << m_agent_name << " on thread: "
					<< std::this_thread::get_id();

				// Coordinator should receive a hello message.
				so_5::send< std::string >( m_mbox, ss.str() );
			}
		};

		// Coordinator's mbox to which hello messages must be sent.
		const auto mbox = so_direct_mbox();

		for( int i = 0; i != 3; ++i )
		{
			coop->make_agent< demo_agent_t >(
					mbox,
					agent_name_prefix + "-" + std::to_string( i ) );
		}

		so_environment().register_coop( std::move( coop ) );
	}
};

int main()
{
	try
	{
		so_5::launch(
			[]( so_5::environment_t & env ) {
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

