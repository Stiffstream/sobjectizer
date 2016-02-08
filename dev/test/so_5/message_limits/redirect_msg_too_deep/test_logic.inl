/*
 * A simple test for message limits (redirecting message with
 * too deep overlimit reaction level).
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

struct msg_request : public so_5::rt::message_t
{
	const so_5::rt::mbox_t m_reply_to;

	msg_request( so_5::rt::mbox_t reply_to )
		: m_reply_to( std::move( reply_to ) )
	{}
};

struct msg_response : public so_5::rt::message_t
{
	const std::string m_reply;

	msg_response( std::string reply ) : m_reply( std::move( reply ) )
	{}
};

struct msg_finish : public so_5::rt::message_t
{
	const so_5::rt::mbox_t m_reply_to;

	msg_finish( so_5::rt::mbox_t reply_to )
		: m_reply_to( std::move( reply_to ) )
	{}
};

struct msg_finish_ack : public so_5::rt::signal_t {};

class a_worker_t : public so_5::rt::agent_t
{
public :
	a_worker_t(
		so_5::rt::environment_t & env,
		std::string reply )
		:	so_5::rt::agent_t( env
				+ limit_then_redirect< msg_request >( 2,
					[this] { return m_self_mbox; } )
				+ limit_then_drop< msg_finish >( 1 ) )
		,	m_reply( std::move( reply ) )
	{}

	void
	set_self_mbox( const so_5::rt::mbox_t & mbox )
	{
		m_self_mbox = mbox;
	}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event( m_self_mbox,
				[&]( const msg_request & evt ) {
					so_5::send< msg_response >( evt.m_reply_to, m_reply );
				} )
			.event( m_self_mbox,
				[&]( const msg_finish & evt ) {
					so_5::send< msg_finish_ack >( evt.m_reply_to );
				} );
	}

private :
	so_5::rt::mbox_t m_self_mbox;
	const std::string m_reply;
};

class a_manager_t : public so_5::rt::agent_t
{
public :
	a_manager_t(
		so_5::rt::environment_t & env,
		std::string expected_response )
		:	so_5::rt::agent_t( env )
		,	m_expected_response( expected_response )
	{}

	void
	set_self_mbox( const so_5::rt::mbox_t & mbox )
	{
		m_self_mbox = mbox;
	}

	void
	set_target_mbox( const so_5::rt::mbox_t & mbox )
	{
		m_target_mbox = mbox;
	}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event( m_self_mbox,
				[&]( const msg_response & evt ) {
					m_responses += evt.m_reply;
				} )
			.event< msg_finish_ack >( m_self_mbox,
				[&] {
					if( m_expected_response == m_responses )
						so_deregister_agent_coop_normally();
					else
						throw std::runtime_error(
								"m_expected_response(" + 
								m_expected_response + ") != "
								"m_responses(" + m_responses + ")" );
				} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< msg_request >( m_target_mbox, m_self_mbox );
		so_5::send< msg_request >( m_target_mbox, m_self_mbox );
		so_5::send< msg_request >( m_target_mbox, m_self_mbox );
		so_5::send< msg_finish >( m_target_mbox, m_self_mbox );
	}

private :
	so_5::rt::mbox_t m_self_mbox;
	so_5::rt::mbox_t m_target_mbox;

	const std::string m_expected_response;
	std::string m_responses;
};

void
do_test(
	const std::string & test_name,
	std::function< void(a_manager_t &, a_worker_t &) > test_tuner )
{
	try
	{
		run_with_time_limit(
			[test_tuner]()
			{
				so_5::launch(
						[test_tuner]( so_5::rt::environment_t & env )
						{
							auto coop = env.create_coop( so_5::autoname );
							
							auto manager = coop->make_agent< a_manager_t >(
									"[one][one]" );

							auto worker = coop->make_agent< a_worker_t >( "[one]" );

							test_tuner( *manager, *worker  );

							env.register_coop( std::move( coop ) );
						} );
			},
			20,
			test_name );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		std::abort();
	}
}

