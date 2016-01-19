#include <iostream>
#include <string>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Ping signal.
// Signal is a special type of message which has no actual data.
// Sending of signals is like sending only atoms in Erlang.
struct ping : public so_5::signal_t {};

// Pong signal.
struct pong : public so_5::signal_t {};

// Message with result of pinger/ponger run.
// Unlike signal message it must have actual data.
struct run_result
{
	std::string m_result;
};

// Pinger agent.
class pinger : public so_5::agent_t
{
public :
	pinger(
		// Working context for agent.
		context_t ctx,
		// Parent's mbox for result sending.
		so_5::mbox_t parent )
		:	so_5::agent_t( ctx )
		,	m_parent( std::move(parent) )
	{}

	// Ponger mbox will be available only after
	// creation of pinger/ponger pair. 
	void set_ponger_mbox( const so_5::mbox_t & mbox ) {
		m_ponger = mbox;
	}

	// This method is automatically called by SObjectizer
	// during agent's registration procedure.
	virtual void so_define_agent() override {
		// Subscription for only one signal.
		so_default_state().event< pong >(
			[this]{
				++m_pongs;
				so_5::send< ping >( m_ponger );
			} );
	}

	// This method is automatically called by SObjectizer
	// just after successful registration.
	// Pinger uses this method to initiate message exchange.
	virtual void so_evt_start() override {
		// Sending signal by corresponding function.
		so_5::send< ping >( m_ponger );
	}

	// This method is automatically called by SObjectizer
	// just after successful agent's deregistration.
	virtual void so_evt_finish() override {
		// Sending result message by corresponding function.
		so_5::send< run_result >(
			// Receiver of the message.
			m_parent,
			// This string will be forwarded to run_result's constructor.
			"pongs: " + std::to_string( m_pongs ) );
	}

private :
	const so_5::mbox_t m_parent;
	so_5::mbox_t m_ponger;
	unsigned int m_pongs = 0;
};

// Ponger agent is very similar to pinger.
// But it hasn't so_evt_start method because it will
// wait the first ping signal from the pinger.
class ponger : public so_5::agent_t
{
public :
	ponger( context_t ctx, so_5::mbox_t parent )
		:	so_5::agent_t( ctx )
		,	m_parent( std::move(parent) )
	{}

	void set_pinger_mbox( const so_5::mbox_t & mbox ) {
		m_pinger = mbox;
	}

	virtual void so_define_agent() override {
		so_default_state().event< ping >(
			[this]{
				++m_pings;
				so_5::send< pong >( m_pinger ); 
			} );
	}

	virtual void so_evt_finish() override {
		so_5::send< run_result >(
			m_parent,
			"pings: " + std::to_string( m_pings ) );
	}

private :
	const so_5::mbox_t m_parent;
	so_5::mbox_t m_pinger;
	unsigned int m_pings = 0;
};

// Parent agent.
// Creates pair of pinger/ponger agents, then limits their working time,
// then handles their run results.
class parent : public so_5::agent_t
{
public :
	parent( context_t ctx ) : so_5::agent_t( ctx )
	{}

	virtual void so_define_agent() override {
		// Arriving of time limit signal means that
		// child cooperation must be deregistered.
		so_default_state().event< stop >( [this] {
				so_environment().deregister_coop(
					// Cooperation name for deregistration.
					"pinger_ponger",
					// The reason of deregistration.
					// This value means that deregistration is caused
					// by application logic.
					so_5::dereg_reason::normal );
			} );

		// NOTE: message type is automatically deduced from
		// event handlers signatures.

		// The first result will be received in default state.
		so_default_state().event( &parent::evt_first_result );
		// But the second one will be received in the next state.
		st_first_result_got.event( &parent::evt_second_result );
	}

	virtual void so_evt_start() override {
		// Creation of child cooperation with pinger and ponger.
		so_5::introduce_child_coop(
				// Parent of the new cooperation.
				*this,
				// Cooperation name.
				"pinger_ponger",
				// Child cooperation will use active_obj dispatcher.
				// So pinger and ponger will work on the different
				// working threads.
				// active_obj dispatcher will be used as a primary
				// dispatcher for that cooperation.
				so_5::disp::active_obj::create_private_disp(
						so_environment() )->binder(),
				// Lambda for tuning cooperation object.
				[this]( so_5::coop_t & coop ) {
					// Filling the child cooperation.
					auto a_pinger = coop.make_agent< pinger >( so_direct_mbox() );
					auto a_ponger = coop.make_agent< ponger >( so_direct_mbox() );

					a_pinger->set_ponger_mbox( a_ponger->so_direct_mbox() );
					a_ponger->set_pinger_mbox( a_pinger->so_direct_mbox() );
				} );

		// Limit the pinger/ponger exchange time.
		so_5::send_delayed< stop >(
				*this, std::chrono::seconds( 1 ) );
	}

private :
	// Time limit signal.
	struct stop : public so_5::signal_t {};

	// Additional state for the agent.
	// This state means that the first result from children agents
	// has been received and that the parent expects the last one.
	const state_t st_first_result_got{ this };

	// Result's accumulator.
	std::string m_results;

	// Event handler for the first result.
	void evt_first_result( const run_result & evt ) {
		m_results = evt.m_result + "; ";

		// Agent's state must be changed.
		this >>= st_first_result_got;
	}

	// Event handler for the next (and last) result.
	void evt_second_result( const run_result & evt ) {
		m_results += evt.m_result;

		// Show the results and finish work.
		std::cout << m_results << std::endl;

		// This is deregistration of the last live cooperation.
		// SO Environment will finish its work.
		so_deregister_agent_coop_normally();
	}
};

int main()
{
	try
	{
		// Create SO Environment objects and run SO Run-Time inside it.
		so_5::launch(
			// SO Environment initialization routine.
			[]( so_5::environment_t & env )
			{
				// We must have the cooperation with just one
				// agent inside it.
				env.register_agent_as_coop(
					// The name for the cooperation will be generated
					// automatically by SO Environment.
					so_5::autoname,
					// The single agent in the cooperation.
					env.make_agent< parent >() );
			} );
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}
}

