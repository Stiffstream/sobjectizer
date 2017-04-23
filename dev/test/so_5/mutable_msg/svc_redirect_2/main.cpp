/*
 * A test for simple service request with mutable message as argument.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

namespace sobj_message {

struct request final : public so_5::message_t
{
	std::string m_data;
	request( std::string data ) : m_data( std::move(data) ) {}
};

class performer final : public so_5::agent_t
{
public :
	performer( context_t ctx ) : so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( &performer::on_request );
	}

private :
	std::string on_request( mhood_t< so_5::mutable_msg<request> > cmd )
	{
		std::string result{ std::move(cmd->m_data) };
		result += "!";
		return result;
	}
};

class provider final : public so_5::agent_t
{
	const so_5::mbox_t m_performer_mbox;

public :
	provider( context_t ctx, so_5::mbox_t performer_mbox )
		: so_5::agent_t( std::move(ctx) )
		, m_performer_mbox( std::move(performer_mbox) )
	{
		so_subscribe_self().event( &provider::on_request );
	}

private :
	std::string on_request( mhood_t< so_5::mutable_msg<request> > cmd )
	{
		auto f = so_5::request_future<std::string>(
				m_performer_mbox, std::move(cmd) );
		return f.get();
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
		ensure( "hello!" == so_5::request_value<
						std::string,
						so_5::mutable_msg<request> >(
				m_provider, so_5::infinite_wait, "hello" ),
			"'hello!' is expected as answer" );

		ensure( "hello_2!" == so_5::request_value<
						std::string,
						so_5::mutable_msg<request> >(
				m_provider, std::chrono::seconds(20), "hello_2" ),
			"'hello_2!' is expected as answer" );

		auto f = so_5::request_future<
						std::string,
						so_5::mutable_msg<request> >(
				m_provider, "hello_3" );
		ensure( "hello_3!" == f.get(), "'hello_3' is expected as answer" );

		so_deregister_agent_coop_normally();
	}
};

void
make_coop( so_5::environment_t & env )
{
	env.introduce_coop(
			so_5::disp::active_obj::create_private_disp( env )->binder(),
			[]( so_5::coop_t & coop ) {
				auto a_performer = coop.make_agent< performer >();
				auto a_provider = coop.make_agent< provider >( a_performer->so_direct_mbox() );
				coop.make_agent< client >( a_provider->so_direct_mbox() );
			} );
}

} /* namespace sobj_message */

namespace user_message {

struct request final
{
	std::string m_data;
	request( std::string data ) : m_data( std::move(data) ) {}
};

class performer final : public so_5::agent_t
{
public :
	performer( context_t ctx ) : so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( &performer::on_request );
	}

private :
	std::string on_request( mhood_t< so_5::mutable_msg<request> > cmd )
	{
		std::string result{ std::move(cmd->m_data) };
		result += "!";
		return result;
	}
};

class provider final : public so_5::agent_t
{
	const so_5::mbox_t m_performer_mbox;

public :
	provider( context_t ctx, so_5::mbox_t performer_mbox )
		: so_5::agent_t( std::move(ctx) )
		, m_performer_mbox( std::move(performer_mbox) )
	{
		so_subscribe_self().event( &provider::on_request );
	}

private :
	std::string on_request( mhood_t< so_5::mutable_msg<request> > cmd )
	{
		auto f = so_5::request_future<std::string>(
				m_performer_mbox, std::move(cmd) );
		return f.get();
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
		ensure( "bye!" == so_5::request_value<
						std::string,
						so_5::mutable_msg<request> >(
				m_provider, so_5::infinite_wait, "bye" ),
			"'bye!' is expected as answer" );

		ensure( "bye_2!" == so_5::request_value<
						std::string,
						so_5::mutable_msg<request> >(
				m_provider, std::chrono::seconds(20), "bye_2" ),
			"'bye_2!' is expected as answer" );

		auto f = so_5::request_future<
						std::string,
						so_5::mutable_msg<request> >(
				m_provider, "bye_3" );
		ensure( "bye_3!" == f.get(), "'bye_3' is expected as answer" );

		so_deregister_agent_coop_normally();
	}
};

void
make_coop( so_5::environment_t & env )
{
	env.introduce_coop(
			so_5::disp::active_obj::create_private_disp( env )->binder(),
			[]( so_5::coop_t & coop ) {
				auto a_performer = coop.make_agent< performer >();
				auto a_provider = coop.make_agent< provider >( a_performer->so_direct_mbox() );
				coop.make_agent< client >( a_provider->so_direct_mbox() );
			} );
}

} /* namespace user_message */

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[&](so_5::environment_t & env) {
						sobj_message::make_coop( env );
						user_message::make_coop( env );
					},
					[](so_5::environment_params_t & params) {
						(void)params;
						params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
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

