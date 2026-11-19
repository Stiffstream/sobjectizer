#include <so_5/all.hpp>

struct msg_ping final : public so_5::signal_t {};
struct msg_pong final : public so_5::signal_t {};

constexpr unsigned int total_exchanges = 3u;

class a_pinger final : public so_5::agent_t
{
	so_5::mbox_t m_ping_mbox;

public:
	a_pinger( context_t ctx )
		: so_5::agent_t{ ctx
				// Support for C++20 coroutines has to be enabled explicitly.
				+ so_5::enable_cpp_coro()
				// Messages have to be redirected to a mchain.
				+ so_5::mchain_as_direct_mbox::use_mchain(
						so_5::create_mchain( ctx.environment() ) )
			}
	{}

	void set_ping_mbox( so_5::mbox_t ping_mbox )
	{
		m_ping_mbox = std::move(ping_mbox);
	}

	so_5::cpp_coro::task_t< void > so_evt_async_start() override
	{
		// The first ping to start exchanges.
		so_5::send< msg_ping >( m_ping_mbox );

		// Receive and handle pongs asynchronously.
		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n( total_exchanges ),
				so_5::receive_case(
					// Extract underlying mchain from our direct mbox.
					so_5::mchain_as_direct_mbox::as_mchain( so_direct_mbox() ),
					[this]( so_5::mhood_t< msg_pong > )
					{
						std::cout << "pong received" << std::endl;
						so_5::send< msg_ping >( m_ping_mbox );
					} ) );

		// The work can be finished now.
		so_deregister_agent_coop_normally();
	}
};

class a_ponger final : public so_5::agent_t
{
	so_5::mbox_t m_pong_mbox;

public:
	a_ponger( context_t ctx )
		: so_5::agent_t{ ctx
				// Support for C++20 coroutines has to be enabled explicitly.
				+ so_5::enable_cpp_coro()
				// Messages have to be redirected to a mchain.
				+ so_5::mchain_as_direct_mbox::use_mchain(
						so_5::create_mchain( ctx.environment() ) )
			}
	{}

	void set_pong_mbox( so_5::mbox_t pong_mbox )
	{
		m_pong_mbox = std::move(pong_mbox);
	}

	so_5::cpp_coro::task_t< void > so_evt_async_start() override
	{
		// Receive and handle pings asynchronously.
		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n( total_exchanges ),
				so_5::receive_case(
					// Extract underlying mchain from our direct mbox.
					so_5::mchain_as_direct_mbox::as_mchain( so_direct_mbox() ),
					[this]( so_5::mhood_t< msg_ping > )
					{
						std::cout << "ping received" << std::endl;
						so_5::send< msg_pong >( m_pong_mbox );
					} ) );
	}
};

int main()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop( []( so_5::coop_t & coop ) {
				// Both agents are bound to the same default one_thread
				// dispatcher. So they will share the same worker thread.
				auto * pinger = coop.make_agent< a_pinger >();
				auto * ponger = coop.make_agent< a_ponger >();

				// The direct mboxes of pinger/ponger are used for interaction.
				pinger->set_ping_mbox( ponger->so_direct_mbox() );
				ponger->set_pong_mbox( pinger->so_direct_mbox() );
			} );
		} );

	return 0;
}

