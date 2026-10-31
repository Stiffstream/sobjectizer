#include <so_5/all.hpp>

#include <so_5/cpp_coro/mchain_select.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

struct msg_ping final : public so_5::signal_t {};
struct msg_pong final : public so_5::signal_t {};
struct msg_completed final : public so_5::signal_t {};

struct communications_t
{
	so_5::mchain_t m_pings;
	so_5::mchain_t m_pongs;
	so_5::mchain_t m_completions;
};

class a_pinger_t final : public so_5::agent_t
{
	communications_t m_communications;

public:
	a_pinger_t( context_t ctx, communications_t communications )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_communications{ std::move(communications) }
	{}

	so_5::cpp_coro::task_t< void >
	so_evt_async_start() override
	{
		so_5::send< msg_ping >( m_communications.m_pings );

		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n(3),
				so_5::receive_case( m_communications.m_pongs,
					[this]( so_5::mhood_t< msg_pong > )
					{
						so_5::send< msg_ping >( m_communications.m_pings );
					} ) );

		so_5::send< msg_completed >( m_communications.m_completions );
	}
};

class a_ponger_t final : public so_5::agent_t
{
	communications_t m_communications;

public:
	a_ponger_t( context_t ctx, communications_t communications )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_communications{ std::move(communications) }
	{}

	so_5::cpp_coro::task_t< void >
	so_evt_async_start() override
	{
		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n(3),
				so_5::receive_case( m_communications.m_pings,
					[this]( so_5::mhood_t< msg_ping > )
					{
						so_5::send< msg_pong >( m_communications.m_pongs );
					} ) );

		so_5::send< msg_completed >( m_communications.m_completions );
	}
};

class a_shutdowner_t final : public so_5::agent_t
{
	communications_t m_communications;

public:
	a_shutdowner_t( context_t ctx, communications_t communications )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_communications{ std::move(communications) }
	{}

	so_5::cpp_coro::task_t< void >
	so_evt_async_start() override
	{
		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n(2),
				so_5::receive_case( m_communications.m_completions,
					[this]( so_5::mhood_t< msg_completed > )
					{
					} ) );

		so_deregister_agent_coop_normally();
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop( []( so_5::coop_t & coop ) {
					communications_t communications{
						.m_pings = so_5::create_mchain( coop.environment() ),
						.m_pongs = so_5::create_mchain( coop.environment() ),
						.m_completions = so_5::create_mchain( coop.environment() )
					};

					coop.make_agent< a_pinger_t >( communications );
					coop.make_agent< a_ponger_t >( communications );
					coop.make_agent< a_shutdowner_t >( communications );
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

