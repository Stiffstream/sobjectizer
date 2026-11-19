#include "../../dummy_yield.hpp"

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <string>

using namespace std::chrono_literals;

namespace test
{

struct msg_a final : public so_5::signal_t {};
struct msg_b final : public so_5::signal_t {};

class a_sender_t final : public so_5::agent_t
{
	struct msg_send_next final : public so_5::signal_t {};

	const so_5::mbox_t m_dest;

	unsigned int m_sent{ 0 };

std::chrono::steady_clock::time_point m_start;
public:
	a_sender_t( context_t ctx, so_5::mbox_t dest )
		: so_5::agent_t{ std::move(ctx) }
		, m_dest{ dest }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_sender_t::evt_send_next );
	}

	void
	so_evt_start() override
	{
		m_start = std::chrono::steady_clock::now();
		so_5::send< msg_send_next >( *this );
	}

	void
	so_evt_finish() override
	{
		std::cout << "sent: " << m_sent << std::endl;
		ensure_or_die( 5u == m_sent,
				"unexpected number of sent messages: "
				+ std::to_string( m_sent ) );
	}

protected:
	void
	evt_send_next( mhood_t< msg_send_next > )
	{
		++m_sent;
		so_5::send< msg_b >( m_dest );

		if( m_sent < 5u )
			so_5::send_delayed< msg_send_next >( *this, 25ms );
	}
};

class a_demo_t final : public so_5::agent_t
{
	unsigned int m_received{ 0 };

public:
	a_demo_t( context_t ctx )
		: so_5::agent_t{
			ctx + so_5::enable_cpp_coro()
			+ limit_then_abort< msg_a >( 1u )
			+ limit_then_drop< msg_b >( 3u )
			}
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_demo_t::evt_a )
			.event( &a_demo_t::evt_b )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< msg_a >( *this );
	}

	void
	so_evt_finish() override
	{
		std::cout << "received: " << m_received << std::endl;
		ensure_or_die( 3u == m_received,
				"unexpected number of received messages: " +
				std::to_string( m_received ) );
	}

private:
	so_5::cpp_coro::task_t< void >
	evt_a( mhood_t< msg_a > )
	{
		co_await dummy_yield( so_environment(), 125ms );
		so_deregister_agent_coop_normally();
	}

	void
	evt_b( mhood_t< msg_b > )
	{
		++m_received;
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop( []( so_5::coop_t & coop ) {
					auto * demo = coop.make_agent< a_demo_t >();
					coop.make_agent< a_sender_t >( demo->so_direct_mbox() );
				} );
		} );
}

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_demo();
			},
			5 );
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

