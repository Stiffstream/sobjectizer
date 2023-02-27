/*
 * Test for correctness of handling subscription/unsubscriptions
 * with delivery filters and message limits.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <so_5/impl/internal_env_iface.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

struct msg_start final : public so_5::signal_t {};

struct msg_unsubscribe final : public so_5::signal_t {};
struct msg_unsubscribed final : public so_5::signal_t {};

struct msg_subscribe final : public so_5::signal_t {};
struct msg_subscribed final : public so_5::signal_t {};

struct msg_complete final : public so_5::signal_t {};

struct msg_first final : public so_5::message_t
{
	int m_v;

	msg_first( int v ) : m_v{v} {}
};

class a_test_t : public so_5::agent_t
{
	const so_5::mbox_t m_mbox;
	int m_index;

public :
	a_test_t( context_t ctx, so_5::mbox_t mbox, int index )
		:	so_5::agent_t( ctx
				+ limit_then_abort< any_unspecified_message >( 100 ) )
		,	m_mbox{ std::move(mbox) }
		,	m_index{ index }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_test_t::evt_start )
			.event( &a_test_t::evt_subscribe )
			.event( &a_test_t::evt_unsubscribe )
			;
	}

private:
	void
	evt_start( mhood_t<msg_start> )
	{
		so_set_delivery_filter(
				m_mbox,
				[]( const msg_first & ) { return true; } );

		so_subscribe( m_mbox )
			.event( &a_test_t::evt_first )
			;
	}

	void
	evt_subscribe( mhood_t<msg_subscribe> )
	{
		so_subscribe( m_mbox )
			.event( &a_test_t::evt_first )
			;

		so_5::send< msg_subscribed >( m_mbox );
	}

	void
	evt_unsubscribe( mhood_t<msg_unsubscribe> )
	{
		so_drop_subscription( m_mbox, &a_test_t::evt_first );

		so_5::send< msg_unsubscribed >( m_mbox );
	}

	void evt_first( mhood_t<msg_first> cmd )
	{
		if( m_index == cmd->m_v )
			throw std::runtime_error{ "message with the same index received" };
	}
};

class a_coordinator_t final : public so_5::agent_t
{
	const so_5::mbox_t m_mbox;
	const so_5::mbox_t m_a1;
	const so_5::mbox_t m_a2;
	const so_5::mbox_t m_a3;

	int m_iterations{};

public:
	a_coordinator_t(
		context_t ctx,
		so_5::mbox_t mbox,
		so_5::mbox_t a1,
		so_5::mbox_t a2,
		so_5::mbox_t a3 )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_mbox{ std::move(mbox) }
		,	m_a1{ std::move(a1) }
		,	m_a2{ std::move(a2) }
		,	m_a3{ std::move(a3) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_coordinator_t::evt_complete )
			;

		so_subscribe( m_mbox )
			.event( &a_coordinator_t::evt_unsubscribed )
			.event( &a_coordinator_t::evt_subscribed )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< msg_start >( m_a1 );
		so_5::send< msg_start >( m_a2 );
		so_5::send< msg_start >( m_a3 );

		do_unsubscription();
	}

private:
	void
	evt_unsubscribed( mhood_t<msg_unsubscribed> )
	{
		so_5::send< msg_first >( m_mbox, 2 );

		do_subscription();
	}

	void
	evt_subscribed( mhood_t<msg_subscribed> )
	{
		++m_iterations;
		if( m_iterations < 10000 )
			do_unsubscription();
		else
			so_5::send< msg_complete >( *this );
	}

	void
	evt_complete( mhood_t<msg_complete> )
	{
		so_deregister_agent_coop_normally();
	}

	void
	do_unsubscription()
	{
		so_5::send< msg_unsubscribe >( m_a2 );
	}

	void
	do_subscription()
	{
		so_5::send< msg_subscribe >( m_a2 );
	}
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop( []( so_5::coop_t & coop ) {
			auto mbox = coop.environment().create_mbox();
			auto a1 = coop.make_agent< a_test_t >( mbox, 1 )->so_direct_mbox();
			auto a2 = coop.make_agent< a_test_t >( mbox, 2 )->so_direct_mbox();
			auto a3 = coop.make_agent< a_test_t >( mbox, 3 )->so_direct_mbox();

			coop.make_agent< a_coordinator_t >( mbox, a1, a2, a3 );
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( init );
			},
			5 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

