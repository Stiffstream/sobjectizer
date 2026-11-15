#include <so_5/all.hpp>

#include <so_5/cpp_coro/mchain_select.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_ping final : public so_5::signal_t {};
struct msg_pong final : public so_5::signal_t {};

constexpr unsigned int total_exchanges = 3u;

class a_pinger final : public so_5::agent_t
{
	so_5::mchain_t m_ping_ch;
	so_5::mchain_t m_pong_ch;

public:
	a_pinger( context_t ctx, so_5::mchain_t ping_ch, so_5::mchain_t pong_ch )
		// Support for C++20 coroutines has to be enabled explicitly.
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_ping_ch{ std::move(ping_ch) }
		, m_pong_ch{ std::move(pong_ch) }
	{}

	so_5::cpp_coro::task_t< void > so_evt_async_start() override
	{
		// The first ping to start exchanges.
		so_5::send< msg_ping >( m_ping_ch );

		// Receive and handle pongs asynchronously.
		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n( total_exchanges ),
				so_5::receive_case( m_pong_ch,
					[this]( so_5::mhood_t< msg_pong > )
					{
						std::cout << "pong received" << std::endl;
						so_5::send< msg_ping >( m_ping_ch );
					} ) );

		// The work can be finished now.
		so_deregister_agent_coop_normally();
	}
};

class a_ponger final : public so_5::agent_t
{
	so_5::mchain_t m_ping_ch;
	so_5::mchain_t m_pong_ch;

public:
	a_ponger( context_t ctx, so_5::mchain_t ping_ch, so_5::mchain_t pong_ch )
		// Support for C++20 coroutines has to be enabled explicitly.
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_ping_ch{ std::move(ping_ch) }
		, m_pong_ch{ std::move(pong_ch) }
	{}

	so_5::cpp_coro::task_t< void > so_evt_async_start() override
	{
		// Receive and handle pings asynchronously.
		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n( total_exchanges ),
				so_5::receive_case( m_ping_ch,
					[this]( so_5::mhood_t< msg_ping > )
					{
						std::cout << "ping received" << std::endl;
						so_5::send< msg_pong >( m_pong_ch );
					} ) );
	}
};

int main()
{
	try
	{
		so_5::launch( []( so_5::environment_t & env ) {
				env.introduce_coop( []( so_5::coop_t & coop ) {
					auto pings_ch = so_5::create_mchain( coop.environment() );
					auto pongs_ch = so_5::create_mchain( coop.environment() );

					// Both agents are bound to the same default one_thread
					// dispatcher. So they will share the same worker thread.
					coop.make_agent< a_pinger >( pings_ch, pongs_ch );
					coop.make_agent< a_ponger >( pings_ch, pongs_ch );
				} );
			} );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

