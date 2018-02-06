/*
 * A test for service requests and deadletter handlers.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

struct first_request final : public so_5::signal_t {};

struct second_request final : public so_5::signal_t {};

class provider_t final : public so_5::agent_t
{
	state_t st_test{ this, "test" };

public :
	provider_t( context_t ctx ) : so_5::agent_t( std::move(ctx) ) {}

	virtual void
	so_define_agent() override
	{
		this >>= st_test;

		st_test.event( [](mhood_t<first_request>) -> std::string {
				return "first";
			} );

		so_subscribe_deadletter_handler(
			so_direct_mbox(),
			[](mhood_t<second_request>) -> std::string {
				return "second";
			} );
	}
};

class consumer_t final : public so_5::agent_t
{
	const so_5::mbox_t m_svc;

public :
	consumer_t( context_t ctx, so_5::mbox_t svc )
		:	so_5::agent_t( std::move(ctx) )
		,	m_svc( std::move(svc) )
	{}

	virtual void
	so_evt_start() override
	{
		ensure_or_die(
				"first" == so_5::request_value< std::string, first_request >(
						m_svc,
						so_5::infinite_wait ),
				"unexpected reply to first_request" );

		ensure_or_die(
				"second" == so_5::request_value< std::string, second_request >(
						m_svc,
						so_5::infinite_wait ),
				"unexpected reply to second_request" );

		so_deregister_agent_coop_normally();
	}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( [&]( so_5::environment_t & env ) {
					env.introduce_coop(
							so_5::disp::active_obj::create_private_disp(env)->binder(),
							[]( so_5::coop_t & coop ) {
								auto provider = coop.make_agent< provider_t >();
								coop.make_agent< consumer_t >( provider->so_direct_mbox() );
							} );
				} );
			},
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 2;
	}

	return 0;
}

