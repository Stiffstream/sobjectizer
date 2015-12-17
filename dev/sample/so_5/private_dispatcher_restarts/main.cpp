/*
 * Demonstration of creation and recreation of big amount of
 * children cooperations with private dispatchers.
 * If dispatchers are not destroyed with cooperation then
 * the example will earn all the system resources very quickly.
 */

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include <so_5/all.hpp>

// Child agent.
//
// Will be member of agent's chain.
// If it is the last member of chain it will deregister
// the whole cooperation.
class a_chain_member_t : public so_5::agent_t
{
public :
	// A signal to be sent from one chain's member to the next member.
	struct msg_your_turn : public so_5::signal_t {};

	a_chain_member_t( context_t ctx ) : so_5::agent_t( ctx )
	{}

	void set_next( const so_5::mbox_t & next )
	{
		m_next = next;
	}

	virtual void so_define_agent() override
	{
		so_default_state().event< msg_your_turn >( [this] {
				if( m_next )
					so_5::send< msg_your_turn >( m_next );
				else
					so_deregister_agent_coop_normally();
			} );
	}

private :
	so_5::mbox_t m_next;
};


// Sample coordinator.
// Works on the default dispatcher.
// Creates and recreates children cooperations.
class a_coordinator_t : public so_5::agent_t
{
public :
	a_coordinator_t( context_t ctx, unsigned int iterations )
		:	so_5::agent_t( ctx )
		,	m_remaining_iterations( iterations )
	{}

	virtual void so_define_agent() override
	{
		// A notification on complete children cooperation deregistration
		// must be received and handled.
		so_default_state().event(
			[this]( const so_5::msg_coop_deregistered & ) {
				--m_remaining_iterations;
				show_remaining_iterations();

				if( m_remaining_iterations )
					// There are some iterations left.
					// New child cooperation must be created and started.
					create_new_child_coop();
				else
				{
					// Example must be finished.
					std::cout << "no more iterations left, finishing..." << std::endl;
					so_deregister_agent_coop_normally();
				}
			} );
	}

	virtual void so_evt_start() override
	{
		show_remaining_iterations();
		create_new_child_coop();
	}

private :
	// How many iterations remains.
	unsigned int m_remaining_iterations;

	void show_remaining_iterations() const
	{
		std::cout << m_remaining_iterations << ": iterations left...\r"
			<< std::flush;
	}

	void create_new_child_coop()
	{
		// The cooperation will use active_obj dispatcher.
		auto disp = so_5::disp::active_obj::create_private_disp(
				so_environment() );
		auto coop = so_5::create_child_coop(
				// This agent will be parent for new cooperation.
				*this,
				// Name for the cooperation will be generated automatically.
				so_5::autoname,
				// The main dispatcher for the new cooperation is
				// the private active_obj dispatcher.
				disp->binder() );
		// We should receive notification about complete
		// child cooperation deregistration.
		coop->add_dereg_notificator(
				so_5::make_coop_dereg_notificator( so_direct_mbox() ) );

		auto first_mbox = fill_coop( *coop );

		so_environment().register_coop( std::move( coop ) );

		// Initial message must be sent to the first chain's member.
		so_5::send< a_chain_member_t::msg_your_turn >( first_mbox );
	}

	// Filling the cooperation with the agents.
	// Return the mbox of the first member in the chain.
	so_5::mbox_t fill_coop( so_5::coop_t & coop )
	{
		const std::size_t agent_count = 8;

		// Those containers are necessary for building agents chain.
		std::vector< so_5::mbox_t > mboxes;
		mboxes.reserve( agent_count );

		std::vector< a_chain_member_t * > agents;
		agents.reserve( agent_count );

		// Create all agents of child cooperation.
		for( std::size_t i = 0; i != agent_count; ++i )
		{
			auto a = coop.make_agent< a_chain_member_t >();
			agents.push_back( a );
			mboxes.push_back( a->so_direct_mbox() );
		}

		// Connect all agents to the chain.
		for( std::size_t i = 0; i != agent_count - 1; ++i )
			agents[ i ]->set_next( mboxes[ i + 1 ] );

		return mboxes[ 0 ];
	}
};

unsigned int detect_iteration_count( int argc, char ** argv )
{
	unsigned int r = 5000;
	if( 2 == argc )
	{
		auto a = std::atoi( argv[ 1 ] );
		if( a > 0 )
			r = static_cast< unsigned int >( a );
		else
			throw std::invalid_argument(
					"invalid iteration count: " + std::to_string( a ) );
	}

	return r;
}

int main( int argc, char ** argv )
{
	try
	{
		const auto iterations = detect_iteration_count( argc, argv );

		so_5::launch(
			[iterations]( so_5::environment_t & env ) {
				// Coordinator agent will work on the default dispatcher.
				env.register_agent_as_coop(
						so_5::autoname,
						env.make_agent< a_coordinator_t >( iterations ) );
			} );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

