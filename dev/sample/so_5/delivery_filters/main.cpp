/*
 * An example of usage of delivery filter.
 */

#include <so_5/all.hpp>

#include <iostream>

using namespace std;
using namespace so_5;

// Message to be filtered.
struct msg_sample
{
	int m_key;
	string m_value;
};

// A signal for doing second part of example.
struct msg_second_part : public signal_t {};

// A signal for finish the example.
struct msg_shutdown : public signal_t {};

// Main example agent.
// Ordinary agent is necessary because a delivery filter can be set
// only by ordinary agent.
class a_example_t : public agent_t
{
public :
	a_example_t( context_t ctx )
		:	agent_t( ctx )
		,	m_mbox( so_environment().create_mbox() )
	{}

	virtual void so_define_agent() override
	{
		so_subscribe_self()
			.event< msg_second_part >( &a_example_t::evt_second_part )
			.event< msg_shutdown >( [this] {
				so_deregister_agent_coop_normally();
			} );
	}

	virtual void so_evt_start() override
	{
		// Subscribe for the message. Without filter.
		so_subscribe( m_mbox ).event( []( const msg_sample & evt ) {
		  cout << "[first]: " << evt.m_key << "-" << evt.m_value << endl;
		} );
		// Sending several messages...
		// All of them will be stored to the agent's queue and handled.
		send< msg_sample >( m_mbox, 0, "only-subscription" );
		send< msg_sample >( m_mbox, 1, "only-subscription" );

		// Setting a delivery filter for message.
		so_set_delivery_filter( m_mbox, []( const msg_sample & evt ) {
		  return 1 == evt.m_key;
		} );
		// Sending several messages...
		// Only one will be stored to the agent's queue and handled.
		send< msg_sample >( m_mbox, 0, "subscription-and-filter" );
		send< msg_sample >( m_mbox, 1, "subscription-and-filter" );

		// Take time for processing already queued messages.
		send< msg_second_part >( *this );
	}

	void evt_second_part()
	{
		// Drop the subscription.
		so_drop_subscription< msg_sample >( m_mbox );
		// Sending several messages...
		// No one of them will be stored to the agent's queue nor handled.
		send< msg_sample >( m_mbox, 0, "only-filter" );
		send< msg_sample >( m_mbox, 1, "only-filter" );

		// Subscribe for the message again.
		so_subscribe( m_mbox ).event( []( const msg_sample & evt ) {
		  cout << "[second]: " << evt.m_key << "-" << evt.m_value << endl;
		} );
		// Sending several messages...
		// Only one will be stored to the agent's queue and handled.
		send< msg_sample >( m_mbox, 0, "subscription-and-filter-2" );
		send< msg_sample >( m_mbox, 1, "subscription-and-filter-2" );

		// Changing the filter to new one.
		so_set_delivery_filter( m_mbox, []( const msg_sample & evt ) {
		  return 0 == evt.m_key;
		} );
		// Sending several messages...
		// Only one will be stored to the agent's queue and handled.
		send< msg_sample >( m_mbox, 0, "subscription-and-filter-3" );
		send< msg_sample >( m_mbox, 1, "subscription-and-filter-3" );

		// Dropping the filter.
		so_drop_delivery_filter< msg_sample >( m_mbox );
		// Sending several messages...
		// All of them will be stored to the agent's queue and handled.
		send< msg_sample >( m_mbox, 0, "only-subscription-2" );
		send< msg_sample >( m_mbox, 1, "only-subscription-2" );

		// Example could be finished.
		send< msg_shutdown >( *this );
	}

private :
	// Separate MPMC-mbox is necessary for delivery filter.
	const mbox_t m_mbox;
};

int main()
{
	try
	{
		so_5::launch( []( environment_t & env ) {
			env.register_agent_as_coop(
					autoname, env.make_agent< a_example_t >() );
		} );

		return 0;
	}
	catch( const std::exception & x )
	{
		cerr << "Exception: " << x.what() << endl;
	}

	return 2;
}

