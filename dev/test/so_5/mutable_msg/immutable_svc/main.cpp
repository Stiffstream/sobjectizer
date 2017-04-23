/*
 * A test for simple service request with immutable message as argument.
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

class provider final : public so_5::agent_t
{
public :
	provider( context_t ctx ) : so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( &provider::on_request );
	}

private :
	std::string on_request( mhood_t< so_5::immutable_msg<request> > cmd )
	{
		return "reply=" + cmd->m_data;
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
		ensure( "reply=hello" == so_5::request_value<
						std::string,
						request >(
				m_provider, so_5::infinite_wait, "hello" ),
			"'reply=hello' is expected" );
		ensure( "reply=imm(hello)" == so_5::request_value<
						std::string,
						so_5::immutable_msg<request> >(
				m_provider, so_5::infinite_wait, "imm(hello)" ),
			"'reply=imm(hello)' is expected" );

		ensure( "reply=hello_2" == so_5::request_value<
						std::string,
						request >(
				m_provider, std::chrono::seconds(20), "hello_2" ),
			"'reply=hello_2' is expected" );
		ensure( "reply=imm(hello_2)" == so_5::request_value<
						std::string,
						so_5::immutable_msg<request> >(
				m_provider, std::chrono::seconds(20), "imm(hello_2)" ),
			"'reply=imm(hello_2)' is expected" );

		{
			auto f = so_5::request_future<
							std::string,
							request >(
					m_provider, "hello_3" );
			ensure( "reply=hello_3" == f.get(), "'reply=hello_3' is expected" );
		}
		{
			auto f = so_5::request_future<
							std::string,
							so_5::immutable_msg<request> >(
					m_provider, "imm(hello_3)" );
			ensure( "reply=imm(hello_3)" == f.get(), "'reply=imm(hello_3)' is expected" );
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
				auto a_provider = coop.make_agent< provider >();
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

class provider final : public so_5::agent_t
{
public :
	provider( context_t ctx ) : so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( &provider::on_request );
	}

private :
	std::string on_request( mhood_t< so_5::immutable_msg<request> > cmd )
	{
		return "reply=" + cmd->m_data;
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
		ensure( "reply=bye" == so_5::request_value<
						std::string,
						request >(
				m_provider, so_5::infinite_wait, "bye" ),
			"'reply=bye' is expected" );
		ensure( "reply=imm(bye)" == so_5::request_value<
						std::string,
						so_5::immutable_msg<request> >(
				m_provider, so_5::infinite_wait, "imm(bye)" ),
			"'reply=imm(bye)' is expected" );

		ensure( "reply=bye_2" == so_5::request_value<
						std::string,
						request >(
				m_provider, std::chrono::seconds(20), "bye_2" ),
			"'reply=bye_2' is expected" );
		ensure( "reply=imm(bye_2)" == so_5::request_value<
						std::string,
						so_5::immutable_msg<request> >(
				m_provider, std::chrono::seconds(20), "imm(bye_2)" ),
			"'reply=imm(bye_2)' is expected" );

		{
			auto f = so_5::request_future<
							std::string,
							request >(
					m_provider, "bye_3" );
			ensure( "reply=bye_3" == f.get(), "'reply=bye_3' is expected" );
		}
		{
			auto f = so_5::request_future<
							std::string,
							so_5::immutable_msg<request> >(
					m_provider, "imm(bye_3)" );
			ensure( "reply=imm(bye_3)" == f.get(), "'reply=imm(bye_3)' is expected" );
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
				auto a_provider = coop.make_agent< provider >();
				coop.make_agent< client >( a_provider->so_direct_mbox() );
			} );
}

} /* namespace user_message */

namespace signal_message {

struct request final : public so_5::signal_t {};

class provider final : public so_5::agent_t
{
	int m_counter{ 0 };
public :
	provider( context_t ctx ) : so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( &provider::on_request );
	}

private :
	int on_request( mhood_t< so_5::immutable_msg<request> > /*cmd*/ )
	{
		return m_counter++;
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
		ensure( 1 == so_5::request_value<
						int,
						so_5::immutable_msg<request> >(
				m_provider, so_5::infinite_wait ),
			"1 is expected" );

		ensure( 2 == so_5::request_value<
						int,
						request >(
				m_provider, std::chrono::seconds(20) ),
			"2 is expected" );
		ensure( 3 == so_5::request_value<
						int,
						so_5::immutable_msg<request> >(
				m_provider, std::chrono::seconds(20) ),
			"3 is expected" );

		{
			auto f = so_5::request_future<
							int,
							request >( m_provider );
			ensure( 4 == f.get(), "4 is expected" );
		}
		{
			auto f = so_5::request_future<
							int,
							so_5::immutable_msg<request> >( m_provider );
			ensure( 5 == f.get(), "5 is expected" );
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
				auto a_provider = coop.make_agent< provider >();
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
						sobj_message::make_coop( env );
						user_message::make_coop( env );
						signal_message::make_coop( env );
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

