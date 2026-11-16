#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test
{

struct msg_a final : public so_5::signal_t {};

class a_receiver_only_t final : public so_5::agent_t
{
	std::string & m_log;

public:
	a_receiver_only_t( context_t ctx, std::string & log )
		: so_5::agent_t{ std::move(ctx) }
		, m_log{ log }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_receiver_only_t::evt_a );
	}

private:
	void
	evt_a( mhood_t< msg_a > )
	{
		m_log += "R;";
	}
};

class a_transmitter_t final : public so_5::agent_t
{
	struct msg_start_sending final : public so_5::signal_t {};

	std::string & m_log;

	const so_5::mbox_t m_receiver;

	unsigned int m_signals_received{};

public:
	a_transmitter_t(
		context_t ctx,
		std::string & log,
		so_5::mbox_t receiver )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro().sequential_limit( 10u ) }
		, m_log{ log }
		, m_receiver{ std::move(receiver) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_transmitter_t::evt_a )
			.event( &a_transmitter_t::evt_start_sending )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< msg_start_sending >( *this );
	}

private:
	void
	send_another_signal()
	{
		so_5::send< msg_a >( *this );
		so_5::send< msg_a >( m_receiver );
	}

	void
	evt_start_sending( mhood_t< msg_start_sending > )
	{
		send_another_signal();
	}

	void
	evt_a( mhood_t< msg_a > )
	{
		m_log += "T;";

		++m_signals_received;
		if( m_signals_received < 20u )
		{
			send_another_signal();
		}
		else
			so_deregister_agent_coop_normally();
	}
};

void
run_demo()
{
	std::string log;

	so_5::launch( [&log]( so_5::environment_t & env ) {
			env.introduce_coop( [&log]( so_5::coop_t & coop ) {
					const auto receiver_mbox = coop.make_agent< a_receiver_only_t >(
							log )->so_direct_mbox();
					coop.make_agent< a_transmitter_t >( log, receiver_mbox );
				} );
		} );

	ensure_or_die(
		"T;T;T;T;T;T;T;T;T;"
		"R;R;R;R;R;R;R;R;R;R;"
		"T;T;T;T;T;T;T;T;T;T;"
		"R;R;R;R;R;R;R;R;R;R;"
		"T;" == log,
		"unexpected log: " + log );
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

