/*
 * A test for redirection of mutable messages.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

class sobj_message_tester final : public so_5::agent_t
{
	struct hello final : public so_5::message_t
	{
		std::string m_content;
		hello( std::string content ) : m_content( std::move(content) ) {}
	};

public :
	sobj_message_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &sobj_message_tester::on_hello );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< so_5::mutable_msg< hello > >( *this, "hello" );
	}

private :
	const hello * m_received_ptr{ nullptr };

	void on_hello( mhood_t< so_5::mutable_msg< hello > > cmd )
	{
		std::cout << "sobj: " << cmd->m_content << std::endl;
		if( !m_received_ptr )
		{
			m_received_ptr = cmd.get();

			cmd->m_content = "bye";
			send( *this, std::move(cmd) );
		}
		else
		{
			ensure( m_received_ptr == cmd.get(), "expect the same message" );
			ensure( "bye" == cmd->m_content, "expect 'bye' message" );

			so_deregister_agent_coop_normally();
		}
	}
};

class user_message_tester final : public so_5::agent_t
{
	struct hello final
	{
		std::string m_content;
		hello( std::string content ) : m_content( std::move(content) ) {}
	};

public :
	user_message_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &user_message_tester::on_hello );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< so_5::mutable_msg< hello > >( *this, "hello" );
	}

private :
	const hello * m_received_ptr{ nullptr };

	void on_hello( mhood_t< so_5::mutable_msg< hello > > cmd )
	{
		std::cout << "user: " << cmd->m_content << std::endl;
		if( !m_received_ptr )
		{
			m_received_ptr = cmd.get();

			cmd->m_content = "bye";
			send( *this, std::move(cmd) );
		}
		else
		{
			ensure( m_received_ptr == cmd.get(), "expect the same message" );
			ensure( "bye" == cmd->m_content, "expect 'bye' message" );

			so_deregister_agent_coop_normally();
		}
	}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[&](so_5::environment_t & env) {
						env.register_agent_as_coop(so_5::autoname,
								env.make_agent<sobj_message_tester>());

						env.register_agent_as_coop(so_5::autoname,
								env.make_agent<user_message_tester>());
					},
					[](so_5::environment_params_t & params) {
						(void)params;
#if 0
						params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
#endif
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

