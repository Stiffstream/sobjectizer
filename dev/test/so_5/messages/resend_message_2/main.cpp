/*
 * Test of resending the same message as a mutable.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

struct message final
{
	std::string m_value;

	message( std::string value ) : m_value{ std::move(value) } {}
};

class first_sender_t final : public so_5::agent_t
{
public :
	first_sender_t( context_t ctx, so_5::mbox_t second )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_second{ std::move(second) }
		,	m_message{ so_5::message_holder_t<message>::make( "hello!" ) }
	{}

	void
	so_evt_start() override
	{
		so_5::send( m_second, m_message );
	}

private :
	const so_5::mbox_t m_second;
	so_5::message_holder_t<message> m_message;
};

class second_sender_t final : public so_5::agent_t
{
public :
	second_sender_t( context_t ctx, so_5::mbox_t third )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_third{ std::move(third) }
	{
		so_subscribe_self().event( &second_sender_t::on_message );
	}

private :
	const so_5::mbox_t m_third;

	void
	on_message( so_5::mhood_t<message> cmd )
	{
		so_5::send( m_third, std::move(cmd) );
	}
};

class third_sender_t final : public so_5::agent_t
{
	struct resend final : public so_5::signal_t {};

public :
	third_sender_t( context_t ctx, so_5::mbox_t fourth )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_fourth{ std::move(fourth) }
	{
		so_subscribe_self()
			.event( &third_sender_t::on_message )
			.event( &third_sender_t::on_resend );
	}

private :
	const so_5::mbox_t m_fourth;
	so_5::message_holder_t< message > m_message;

	void
	on_message( so_5::mhood_t<message> cmd )
	{
		m_message = cmd.make_holder();

		so_5::send<resend>( *this );
	}

	void
	on_resend( so_5::mhood_t<resend> )
	{
		so_5::send( m_fourth, std::move(m_message) );
	}
};

class last_t final : public so_5::agent_t
{
public :
	last_t( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
	{
		so_subscribe_self().event( &last_t::on_message );
	}

private :
	void
	on_message( so_5::mhood_t<message> cmd )
	{
		ensure_or_die( cmd->m_value == "hello!",
				"unexpected value: " + cmd->m_value );

		so_deregister_agent_coop_normally();
	}
};

int
main()
{
	run_with_time_limit( [] {
			so_5::launch(
				[]( so_5::environment_t & env )
				{
					env.introduce_coop( []( so_5::coop_t & coop ) {
							auto last = coop.make_agent< last_t >();
							auto third = coop.make_agent< third_sender_t >(
									last->so_direct_mbox() );
							auto second = coop.make_agent< second_sender_t >(
									third->so_direct_mbox() );
							coop.make_agent< first_sender_t >(
									second->so_direct_mbox() );
						} );
				},
				[]( so_5::environment_params_t & params )
				{
					params.message_delivery_tracer(
							so_5::msg_tracing::std_cout_tracer() );
				} );
		},
		10 );

	return 0;
}

