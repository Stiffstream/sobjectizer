/*
 * A simple test for message limits (dropping the message at message peaks).
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

struct msg_ping : public so_5::signal_t {};
struct msg_pong : public so_5::signal_t {};

struct msg_finish : public so_5::signal_t {};

class a_sender_t : public so_5::agent_t
{
public :
	a_sender_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	void
	set_receiver( const so_5::mbox_t & mbox )
	{
		m_receiver = mbox;
	}

	virtual void
	so_define_agent() override
	{
		so_default_state().event< msg_pong >(
			[&] {
				const unsigned int max_series = 5;

				++m_pongs;
				if( !( m_pongs % 2) )
				{
					if( m_series_sent < max_series )
						send_pings();
					else if( m_series_sent == max_series )
						so_5::send_to_agent< msg_finish >( *this );
				}
			} );

		so_default_state().event< msg_finish >(
			[&] {
				const auto expected = m_series_sent * 2;
				if( m_pongs == expected )
					so_deregister_agent_coop_normally();
				else
					throw std::runtime_error( "pongs count mismatch; "
							"expected: " + std::to_string( expected ) +
							", received: " + std::to_string( m_pongs ) );
			} );
	}

	virtual void
	so_evt_start() override
	{
		send_pings();
	}

private :
	so_5::mbox_t m_receiver;

	unsigned int m_series_sent = 0;
	unsigned int m_pongs = 0;

	void
	send_pings()
	{
		so_5::send< msg_ping >( m_receiver );
		so_5::send< msg_ping >( m_receiver );
		so_5::send< msg_ping >( m_receiver );

		++m_series_sent;
	}
};

class a_receiver_t : public so_5::agent_t
{
public :
	a_receiver_t(
		context_t ctx,
		so_5::mbox_t sender )
		:	so_5::agent_t( ctx + limit_then_drop< msg_ping >( 2 ) )
		,	m_sender( std::move( sender ) )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event< msg_ping >( [&]{ so_5::send< msg_pong >( m_sender ); } );
	}

private :
	const so_5::mbox_t m_sender;
};

void
init( so_5::environment_t & env )
{
	auto coop = env.create_coop( so_5::autoname );
	auto sender = coop->make_agent< a_sender_t >();
	auto receiver = coop->make_agent< a_receiver_t >( sender->so_direct_mbox() );
	sender->set_receiver( receiver->so_direct_mbox() );

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
			"simple message drop at peaks test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

