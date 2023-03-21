#include <so_5/unique_subscribers_mbox.hpp>
#include <so_5/all.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>
#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct test_message final : public so_5::message_t
{
	std::string m_data;

	explicit test_message( std::string data ) : m_data{ std::move(data) }
	{}
};

struct try_1 final : public so_5::signal_t {};
struct try_2 final : public so_5::signal_t {};

class first final : public so_5::agent_t
{
	const so_5::mbox_t m_test_mbox;
	so_5::mbox_t m_second_mbox;

public:
	first( context_t ctx, so_5::mbox_t test_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_test_mbox{ std::move(test_mbox) }
	{}

	void
	set_second_mbox( so_5::mbox_t mbox )
	{
		m_second_mbox = std::move(mbox);
	}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &first::evt_try_1 )
			;
	}

	void
	so_evt_start() override
	{
		so_subscribe( m_test_mbox )
			.event( &first::evt_test_message )
			;

		so_5::send< test_message >( m_test_mbox, "to first" );
	}

private:
	void
	evt_test_message( mhood_t< test_message > cmd )
	{
		std::cout << "first.evt_test_message: " << cmd->m_data << std::endl;
		so_5::send< try_1 >( m_second_mbox );
	}

	void
	evt_try_1( mhood_t< try_1 > )
	{
		so_drop_subscription_for_all_states(
				m_test_mbox,
				&first::evt_test_message );

		so_5::send< try_2 >( m_second_mbox );
	}
};

class second final : public so_5::agent_t
{
	const so_5::mbox_t m_test_mbox;

	so_5::mbox_t m_first_mbox;

public:
	second( context_t ctx, so_5::mbox_t test_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_test_mbox{ std::move(test_mbox) }
	{}

	void
	set_first_mbox( so_5::mbox_t mbox )
	{
		m_first_mbox = std::move(mbox);
	}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &second::evt_try_1 )
			.event( &second::evt_try_2 )
			;
	}

private:
	void
	evt_try_1( mhood_t< try_1 > )
	{
		bool exception_caught = false;
		try
		{
			so_subscribe( m_test_mbox )
				.event( &second::evt_test_message )
				;
		}
		catch( const so_5::exception_t & x )
		{
			std::cout << "exception with error_code=" << x.error_code() << std::endl;
			exception_caught =
					(x.error_code() == so_5::rc_evt_handler_already_provided);
		}

		if( !exception_caught )
			throw std::runtime_error{ "expected exception isn't thrown!" };

		so_5::send< try_1 >( m_first_mbox );
	}

	void
	evt_try_2( mhood_t< try_2 > )
	{
		so_subscribe( m_test_mbox )
			.event( &second::evt_test_message )
			;

		so_5::send< test_message >( m_test_mbox, "to second" );
	}

	void
	evt_test_message( mhood_t< test_message > cmd )
	{
		std::cout << "second.evt_test_message: " << cmd->m_data << std::endl;

		so_deregister_agent_coop_normally();
	}
};

UT_UNIT_TEST( simple_case )
{
	run_with_time_limit( [] {
			so_5::launch( [&](so_5::environment_t & env) {
						auto test_mbox = so_5::make_unique_subscribers_mbox( env );

						env.introduce_coop( [&]( so_5::coop_t & coop ) {
								auto * f = coop.make_agent< first >( test_mbox );
								auto * s = coop.make_agent< second >( test_mbox );
								f->set_second_mbox( s->so_direct_mbox() );
								s->set_first_mbox( f->so_direct_mbox() );
							} );
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

