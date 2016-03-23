/*
 * A very simple demo of wrapped_env_t and mchains.
 */

#include <iostream>
#include <string>

// Main SObjectizer header file.
#include <so_5/all.hpp>

using namespace std;
using namespace std::chrono;

// A supervisor agent which will interact with main thread.
class a_supervisor : public so_5::agent_t
{
	const state_t st_idle{ this };
	const state_t st_started{ this };
	const state_t st_finished{ this };

	using clock = steady_clock;

public :
	// A message for asking current state of the supervisor.
	struct ask_status {};

	// Messages about status of the supervisor.
	struct status_idle {};
	struct status_in_progress {};
	struct status_finished { unsigned long long m_ms; };

	// A message to start ping-pong.
	struct start_ping_pong { unsigned int m_pings; };

	// A message to indicate that pinger or ponger stopped its work.
	struct ping_pong_stopped {};

	a_supervisor(
		// Working context for the agent.
		context_t ctx,
		// Mbox for requests from not-SO-part.
		so_5::mbox_t req_mbox,
		// Chain for responses to not-SO-part.
		so_5::mchain_t chain )
		:	so_5::agent_t{ ctx }
		,	m_req_mbox{ move(req_mbox) }
		,	m_chain{ move(chain) }
		{}

	virtual void
	so_define_agent() override
		{
			// Will start in idle state.
			this >>= st_idle;

			// Event handlers for idle state.
			st_idle
				// On status request will sent status_idle to chain as a reply.
				.event( m_req_mbox, [this]( ask_status ) {
					so_5::send< status_idle >( m_chain );
				} )
				// Ping-pong start requires more actions so it is implemented
				// as separate method.
				.event( m_req_mbox, &a_supervisor::evt_start_ping_pong );

			st_started
				// On status request will reply by sending
				// status_in_progress to the chain.
				.event( m_req_mbox, [this]( ask_status ) {
					so_5::send< status_in_progress >( m_chain );
				} )
				// Reaction to the end of ping-pong.
				.event( [this]( ping_pong_stopped ) {
					// Storing time for the future ask_status requests.
					m_last_duration_ms = static_cast< unsigned long long >(
							duration_cast< milliseconds >(
									clock::now() - m_started_at ).count() );

					// Reflect the end of ping-pong by changing state of supervisor.
					this >>= st_finished;
					// Send the result of ping-pong to the main thread.
					so_5::send< status_finished >( m_chain, m_last_duration_ms );
				} );

			st_finished
				// On status request will reply by sending
				// status_finished to the chain.
				.event( m_req_mbox, [this]( ask_status ) {
					so_5::send< status_finished >( m_chain, m_last_duration_ms );
				} )
				// Star of ping-pong will be handled exactly same way
				// as in st_idle state.
				.event( m_req_mbox, &a_supervisor::evt_start_ping_pong );
		}

private :
	const so_5::mbox_t m_req_mbox;
	const so_5::mchain_t m_chain;

	clock::time_point m_started_at;
	unsigned long long m_last_duration_ms;

	void evt_start_ping_pong( start_ping_pong evt )
	{
		this >>= st_started;

		m_started_at = clock::now();

		// Pinger and ponger will be started inside a new coop.
		// They will work on different threads (management of those
		// threads will be done by private active_obj dispatcher).
		so_5::introduce_child_coop( *this,
				so_5::disp::active_obj::create_private_disp(
						so_environment() )->binder(),
				[this, evt]( so_5::coop_t & coop )
				{
					// Types of messages to be used by pinger and ponger.
					struct ping{ unsigned int m_v; };
					struct pong{ unsigned int m_v; };

					// Pinger and ponger itself.
					auto pinger = coop.define_agent();
					auto ponger = coop.define_agent();

					// Pinger should know mbox of its parent agent.
					auto parent_mbox = so_direct_mbox();

					// Pinger will send the first ping on the start...
					pinger.on_start( [ponger, evt] {
							so_5::send< ping >( ponger, evt.m_pings );
						} )
						// ...and the next ping on the pong from ponger agent.
						.event( pinger,
							[ponger, parent_mbox, &coop]( pong reply ) {
							if( reply.m_v )
								so_5::send< ping >( ponger, reply.m_v - 1 );
							else
							{
								so_5::send< ping_pong_stopped >( parent_mbox );
								coop.deregister_normally();
							}
						} );

					// Ponger will handle just one ping message.
					ponger.event( ponger, [pinger]( ping req ) {
							so_5::send< pong >( pinger, req.m_v );
						} );
				} );
	}
};

// Main demo loop.
void demo()
{
	// SO Environment in a special wrapper object.
	// Environment will be started automatically.
	so_5::wrapped_env_t sobj;

	// Mbox for sending requests to SO-part of the app.
	auto req_mbox = sobj.environment().create_mbox();
	// Chain for receiving replies from SO-part of the app. 
	auto chain = create_mchain( sobj );

	// Start SO-part of the app.
	sobj.environment().introduce_coop( [&]( so_5::coop_t & coop ) {
			coop.make_agent< a_supervisor >( req_mbox, chain );
		} );

	// Main loop. The demo will be finished when user enter
	// 'exit' or 'quit' command.
	while( true )
	{
		cout << "Enter command (status,start,exit): " << flush;
		string choice;
		cin >> choice;

		if( "status" == choice )
		{
			// Ask status request must be sent.
			so_5::send< a_supervisor::ask_status >( req_mbox );
			// Now we can receive and handle responses.
			// Wait for messages no more than 200ms.
			auto r = receive(
					from( chain ).empty_timeout( milliseconds(200) ),
					[]( a_supervisor::status_idle ) {
						cout << "status: IDLE" << endl;
					},
					[]( a_supervisor::status_in_progress ) {
						cout << "status: IN PROGRESS" << endl;
					},
					[]( a_supervisor::status_finished v ) {
						cout << "status: finished in " << v.m_ms << "ms" << endl;
					} );
			// Have some messages been handled?
			if( !r.handled() )
				cout << "--- no response from supervisor ---" << endl;
		}
		else if( "start" == choice )
		{
			cout << "Enter pings count: " << flush;
			unsigned int pings = 0;
			cin >> pings;

			// Initiate ping-pong.
			so_5::send< a_supervisor::start_ping_pong >( req_mbox, pings );
		}
		else if( "exit" == choice || "quit" == choice )
			break;
	}

	// SO Environment will be automatically stopped.
}

int main()
{
	try
	{
		demo();
	}
	catch( const exception & x )
	{
		cerr << "Exception: " << x.what() << endl;
		return 2;
	}

	return 0;
}

