/*
 * A test for redirection svc_requests with signals.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

namespace signal_message {

struct request final : public so_5::signal_t {};

class performer final : public so_5::agent_t
{
	int m_counter{ 0 };
public :
	performer( context_t ctx ) : so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( &performer::on_request );
	}

private :
	int on_request( mhood_t< so_5::immutable_msg<request> > /*cmd*/ )
	{
		return m_counter++;
	}
};

class provider final : public so_5::agent_t
{
	const so_5::mbox_t m_performer1;
	const so_5::mbox_t m_performer2;
public :
	provider( context_t ctx,
		so_5::mbox_t performer1,
		so_5::mbox_t performer2 )
		: so_5::agent_t( std::move(ctx) )
		, m_performer1( std::move(performer1) )
		, m_performer2( std::move(performer2) )
	{
		so_subscribe_self().event( &provider::on_request );
	}

private :
	int on_request( mhood_t< so_5::immutable_msg<request> > cmd )
	{
		return so_5::request_value< int >(
						m_performer1, so_5::infinite_wait, cmd )
				+ so_5::request_value< int >(
						m_performer2, std::chrono::milliseconds(200), cmd );
	}
};

class client final : public so_5::agent_t
{
	const so_5::mbox_t m_provider;

public :
	client( context_t ctx, so_5::mbox_t provider_mbox )
		:	so_5::agent_t(std::move(ctx))
		,	m_provider(std::move(provider_mbox))
	{}

	virtual void
	so_evt_start() override
	{
		ensure( 0 == so_5::request_value<
						int,
						request >(
				m_provider, so_5::infinite_wait ),
			"0 is expected" );
		ensure( 2 == so_5::request_value<
						int,
						so_5::immutable_msg<request> >(
				m_provider, so_5::infinite_wait ),
			"2 is expected" );

		ensure( 4 == so_5::request_value<
						int,
						request >(
				m_provider, std::chrono::seconds(20) ),
			"4 is expected" );
		ensure( 6 == so_5::request_value<
						int,
						so_5::immutable_msg<request> >(
				m_provider, std::chrono::seconds(20) ),
			"6 is expected" );

		{
			auto f = so_5::request_future<
							int,
							request >( m_provider );
			ensure( 8 == f.get(), "8 is expected" );
		}
		{
			auto f = so_5::request_future<
							int,
							so_5::immutable_msg<request> >( m_provider );
			ensure( 10 == f.get(), "10 is expected" );
		}

		so_deregister_agent_coop_normally();
	}
};

void
make_coop( so_5::environment_t & env )
{
	env.introduce_coop(
			so_5::disp::active_obj::create_private_disp( env )->binder(),
			[]( so_5::coop_t & coop ) {
				auto a_performer1 = coop.make_agent< performer >();
				auto a_performer2 = coop.make_agent< performer >();
				auto a_provider = coop.make_agent< provider >(
						a_performer1->so_direct_mbox(),
						a_performer2->so_direct_mbox() );
				coop.make_agent< client >( a_provider->so_direct_mbox() );
			} );
}

} /* namespace signal_message */

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[&](so_5::environment_t & env) {
						signal_message::make_coop( env );
					},
					[](so_5::environment_params_t & params) {
						(void)params;
//						params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
					} );
			},
			5,
			"simple agent");
	}
	catch(const exception & ex)
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

