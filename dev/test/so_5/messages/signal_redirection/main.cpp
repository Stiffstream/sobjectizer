/*
 * Test for redirection of a signal.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct test_signal final : public so_5::signal_t {};

class receiver_t final : public so_5::agent_t
{
public:
	using so_5::agent_t::agent_t;

	void
	so_define_agent() override
	{
		so_subscribe_self().event(
				[this](mhood_t<test_signal>) {
					so_deregister_agent_coop_normally();
				} );
	}
};

class sender_t final : public so_5::agent_t
{
	const so_5::mbox_t m_receiver;

	void
	evt_test_signal( mhood_t<test_signal> cmd )
	{
		so_5::send( m_receiver, cmd );
	}

public:
	sender_t( context_t ctx, so_5::mbox_t receiver )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_receiver{ std::move(receiver) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self().event( &sender_t::evt_test_signal );
	}

	void
	so_evt_start() override
	{
		so_5::send<test_signal>( *this );
	}
};

int
main()
{
	run_with_time_limit( [] {
				so_5::launch( []( so_5::environment_t & env ) {
						env.introduce_coop( []( so_5::coop_t & coop ) {
								auto receiver = coop.make_agent< receiver_t >()
										->so_direct_mbox();

								coop.make_agent< sender_t >( receiver );
							} );
					} );
			},
			5 );

	return 0;
}

