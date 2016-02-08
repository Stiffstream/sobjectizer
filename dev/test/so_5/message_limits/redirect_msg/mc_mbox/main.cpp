/*
 * A simple test for message limits (redirecting message).
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_request : public so_5::message_t
{
	const so_5::mbox_t m_reply_to;

	msg_request( so_5::mbox_t reply_to )
		: m_reply_to( std::move( reply_to ) )
	{}
};

struct msg_response : public so_5::message_t
{
	const std::string m_reply;

	msg_response( std::string reply ) : m_reply( std::move( reply ) )
	{}
};

class a_worker_t : public so_5::agent_t
{
public :
	a_worker_t(
		so_5::environment_t & env,
		so_5::mbox_t self_mbox,
		std::string reply )
		:	so_5::agent_t( env + limit_then_drop< msg_request >( 1 ) )
		,	m_self_mbox( std::move( self_mbox ) )
		,	m_reply( std::move( reply ) )
	{}

	a_worker_t(
		so_5::environment_t & env,
		so_5::mbox_t self_mbox,
		std::string reply,
		const so_5::mbox_t & redirect_to )
		:	so_5::agent_t( env
				+ limit_then_redirect< msg_request >( 1,
						[redirect_to] { return redirect_to; } ) )
		,	m_self_mbox( std::move( self_mbox ) )
		,	m_reply( std::move( reply ) )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state().event(
			m_self_mbox,
			[&]( const msg_request & evt ) {
				so_5::send< msg_response >( evt.m_reply_to, m_reply );
			} );
	}

private :
	const so_5::mbox_t m_self_mbox;
	const std::string m_reply;
};

class a_manager_t : public so_5::agent_t
{
public :
	a_manager_t(
		so_5::environment_t & env,
		so_5::mbox_t self_mbox,
		so_5::mbox_t target_mbox,
		std::string expected_response )
		:	so_5::agent_t( env )
		,	m_self_mbox( std::move( self_mbox ) )
		,	m_target_mbox( std::move( target_mbox ) )
		,	m_expected_response( expected_response )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state().event(
			m_self_mbox,
			[&]( const msg_response & evt ) {
				m_responses += evt.m_reply;

				if( m_expected_response == m_responses )
					so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< msg_request >( m_target_mbox, m_self_mbox );
		so_5::send< msg_request >( m_target_mbox, m_self_mbox );
		so_5::send< msg_request >( m_target_mbox, m_self_mbox );
	}

private :
	const so_5::mbox_t m_self_mbox;
	const so_5::mbox_t m_target_mbox;
	const std::string m_expected_response;
	std::string m_responses;
};

void
init( so_5::environment_t & env )
{
	auto w1_mbox = env.create_mbox();
	auto w2_mbox = env.create_mbox();
	auto w3_mbox = env.create_mbox();
	auto m_mbox = env.create_mbox();

	auto coop = env.create_coop( so_5::autoname );
	
	coop->make_agent< a_manager_t >( m_mbox, w1_mbox, "[one][two][three]" );

	coop->make_agent< a_worker_t >( w1_mbox, "[one]", w2_mbox );
	coop->make_agent< a_worker_t >( w2_mbox, "[two]", w3_mbox );
	coop->make_agent< a_worker_t >( w3_mbox, "[three]" );

	env.register_coop( std::move( coop ) );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			20,
			"simple message redirect on MPMC-mboxes test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

