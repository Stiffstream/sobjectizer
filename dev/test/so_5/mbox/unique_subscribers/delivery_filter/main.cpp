#include <so_5/unique_subscribers_mbox.hpp>
#include <so_5/all.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>
#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

struct msg_value_a final : public so_5::message_t
{
	int m_a;

	msg_value_a( int a ) : m_a{ a } {}
};

struct msg_value_b final : public so_5::message_t
{
	int m_b;

	msg_value_b( int b ) : m_b{ b } {}
};

class first final : public so_5::agent_t
{
	const so_5::mbox_t m_test_mbox;

	std::string m_protocol;

public:
	first( context_t ctx, so_5::mbox_t test_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_test_mbox{ std::move(test_mbox) }
	{}

	void
	so_define_agent() override
	{
		so_set_delivery_filter( m_test_mbox,
			[]( const msg_value_a & msg ) {
				return (msg.m_a > 0) && (msg.m_a < 10);
			} );

		so_subscribe( m_test_mbox )
			.event( [this]( mhood_t<msg_value_a> cmd ) {
					m_protocol += std::to_string( cmd->m_a ) + ";";
				} )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< msg_value_a >( m_test_mbox, -1 );
		so_5::send< msg_value_a >( m_test_mbox, 0 );
		so_5::send< msg_value_a >( m_test_mbox, 1 );
		so_5::send< msg_value_a >( m_test_mbox, 2 );
		so_5::send< msg_value_a >( m_test_mbox, 9 );
		so_5::send< msg_value_a >( m_test_mbox, 10 );
		so_5::send< msg_value_a >( m_test_mbox, 11 );

		so_deregister_agent_coop_normally();
	}

	void
	so_evt_finish() override
	{
		const std::string expected_protocol = "1;2;9;";
		ensure_or_die( expected_protocol == m_protocol,
				"unexpected protocol for 'first', expected='" + expected_protocol +
				"', actual='" + m_protocol + "'" );
	}
};

class second final : public so_5::agent_t
{
	const so_5::mbox_t m_test_mbox;

	std::string m_protocol;

public:
	second( context_t ctx, so_5::mbox_t test_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_test_mbox{ std::move(test_mbox) }
	{}

	void
	so_define_agent() override
	{
		so_set_delivery_filter_for_mutable_msg( m_test_mbox,
			[]( const msg_value_a & msg ) {
				return (msg.m_a > 0) && (msg.m_a < 10);
			} );

		so_subscribe( m_test_mbox )
			.event( [this]( mutable_mhood_t<msg_value_a> cmd ) {
					m_protocol += std::to_string( cmd->m_a ) + ";";
				} )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< so_5::mutable_msg<msg_value_a> >( m_test_mbox, -1 );
		so_5::send< so_5::mutable_msg<msg_value_a> >( m_test_mbox, 0 );
		so_5::send< so_5::mutable_msg<msg_value_a> >( m_test_mbox, 1 );
		so_5::send< so_5::mutable_msg<msg_value_a> >( m_test_mbox, 2 );
		so_5::send< so_5::mutable_msg<msg_value_a> >( m_test_mbox, 9 );
		so_5::send< so_5::mutable_msg<msg_value_a> >( m_test_mbox, 10 );
		so_5::send< so_5::mutable_msg<msg_value_a> >( m_test_mbox, 11 );

		so_drop_delivery_filter< so_5::mutable_msg<msg_value_a> >( m_test_mbox );
		so_5::send< so_5::mutable_msg<msg_value_a> >( m_test_mbox, -5 );

		so_deregister_agent_coop_normally();
	}

	void
	so_evt_finish() override
	{
		const std::string expected_protocol = "1;2;9;-5;";
		ensure_or_die( expected_protocol == m_protocol,
				"unexpected protocol for 'second', expected='" + expected_protocol +
				"', actual='" + m_protocol + "'" );
	}
};

class third final : public so_5::agent_t
{
	const so_5::mbox_t m_test_mbox;

	std::string m_protocol;

public:
	third( context_t ctx, so_5::mbox_t test_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_test_mbox{ std::move(test_mbox) }
	{}

	void
	so_define_agent() override
	{
		so_set_delivery_filter( m_test_mbox,
			[]( const msg_value_b & msg ) {
				return (msg.m_b > 0) && (msg.m_b < 10);
			} );

		so_subscribe( m_test_mbox )
			.event( [this]( mhood_t<msg_value_b> cmd ) {
					m_protocol += std::to_string( cmd->m_b ) + ";";
				} )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< msg_value_b >( m_test_mbox, -1 );
		so_5::send< msg_value_b >( m_test_mbox, 0 );
		so_5::send< msg_value_b >( m_test_mbox, 1 );
		so_5::send< msg_value_b >( m_test_mbox, 2 );
		so_5::send< msg_value_b >( m_test_mbox, 9 );
		so_5::send< msg_value_b >( m_test_mbox, 10 );
		so_5::send< msg_value_b >( m_test_mbox, 11 );

		so_drop_delivery_filter< msg_value_b >( m_test_mbox );
		so_5::send< msg_value_b >( m_test_mbox, 12 );

		so_deregister_agent_coop_normally();
	}

	void
	so_evt_finish() override
	{
		const std::string expected_protocol = "1;2;9;12;";
		ensure_or_die( expected_protocol == m_protocol,
				"unexpected protocol for 'third', expected='" + expected_protocol +
				"', actual='" + m_protocol + "'" );
	}
};

UT_UNIT_TEST( simple_case )
{
	run_with_time_limit( [] {
			so_5::launch( [&](so_5::environment_t & env) {
						auto test_mbox = so_5::make_unique_subscribers_mbox( env );

						env.register_agent_as_coop(
								env.make_agent< first >( test_mbox ),
								so_5::disp::one_thread::make_dispatcher( env ).binder() );

						env.register_agent_as_coop(
								env.make_agent< second >( test_mbox ),
								so_5::disp::one_thread::make_dispatcher( env ).binder() );

						env.register_agent_as_coop(
								env.make_agent< third >( test_mbox ),
								so_5::disp::one_thread::make_dispatcher( env ).binder() );
					},
					[](so_5::environment_params_t & params) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					} );
		},
		5 );
}

int main()
{
	UT_RUN_UNIT_TEST( simple_case )
}

