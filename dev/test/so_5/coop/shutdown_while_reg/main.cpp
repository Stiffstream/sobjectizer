/*
 * A unit-test for checking shutdown of SOEnv while coop registration
 * is in progress.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

struct check_context_t
{
	std::promise<void> m_hello_received;
};

class small_actor_t final : public so_5::agent_t
{
	struct hello final : public so_5::signal_t {};

	bool m_evt_start_completed{ false };
	bool m_hello_received{ false };

	check_context_t & m_check_context;

public :
	small_actor_t(
		context_t ctx,
		check_context_t & check_context )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_check_context{ check_context }
	{}

	virtual ~small_actor_t() override
	{
		ensure_or_die( m_evt_start_completed,
				"!m_evt_start_completed" );

		ensure_or_die( m_hello_received,
				"!m_hello_received" );
	}

	void
	so_define_agent() override
	{
		so_subscribe_self().event( &small_actor_t::on_hello );

		so_environment().stop();
	}

	void
	so_evt_start() override
	{
		m_evt_start_completed = true;
		so_5::send_delayed< hello >( *this, std::chrono::milliseconds{125} );
	}

private :
	void
	on_hello( mhood_t<hello> )
	{
		m_hello_received = true;
		m_check_context.m_hello_received.set_value();
	}
};

class driver_t final : public so_5::agent_t
{
public :
	driver_t( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
	{}

	void
	so_evt_start() override
	{
		so_environment().introduce_coop(
				so_5::disp::one_thread::make_dispatcher( so_environment() )
						.binder(),
				[]( so_5::coop_t & coop ) {
					auto ctx = coop.take_under_control(
							std::make_unique< check_context_t >() );

					coop.make_agent< small_actor_t >( std::ref(*ctx) );
					coop.add_reg_notificator(
							[ctx](
								so_5::environment_t &,
								const so_5::coop_handle_t & ) noexcept {
								// Wait while small_actor receive hello message.
								ctx->m_hello_received.get_future().wait();
							} );
				} );
	}
};

int
main()
{
	run_with_time_limit( [] {
			so_5::launch( []( so_5::environment_t & env ) {
					env.register_agent_as_coop( env.make_agent< driver_t >() );
				} );
		},
		10 );

	return 0;
}

