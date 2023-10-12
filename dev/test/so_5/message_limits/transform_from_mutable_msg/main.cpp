/*
 * A test for trasformation of a mutable message into another mutable message.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_initial_message final : public so_5::message_t
{
	const std::string m_msg;

	msg_initial_message( std::string msg ) : m_msg{ std::move(msg) }
	{}
};

struct msg_transformed_message final : public so_5::message_t
{
	std::string m_msg;

	msg_transformed_message( std::string msg ) : m_msg{ std::move(msg) }
	{}
};

class agent_with_limit_t final : public so_5::agent_t
{
public :
	agent_with_limit_t(
		context_t ctx )
		:	so_5::agent_t{ ctx
				+ limit_then_transform< so_5::mutable_msg< msg_initial_message > >( 1u,
						[this]( msg_initial_message & msg ) {
							return make_transformed<
										so_5::mutable_msg< msg_transformed_message > >(
									so_direct_mbox(),
									"<" + msg.m_msg + ">" );
						} )
				+ limit_then_abort< so_5::mutable_msg< msg_transformed_message > >( 1u )
			}
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( [this]( mutable_mhood_t<msg_initial_message> cmd ) {
					m_received += "[initial:" + cmd->m_msg + "]";
				} )
			.event( [this]( mutable_mhood_t<msg_transformed_message> cmd ) {
					m_received += "[transformed:" + cmd->m_msg + "]";

					const std::string expected =
						"[initial:hello][transformed:<bye>]";

					if( expected != m_received )
						throw std::runtime_error( expected + " != " + m_received );
					else
						so_deregister_agent_coop_normally();
				} )
			;
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< so_5::mutable_msg<msg_initial_message> >( *this, "hello" );
		so_5::send< so_5::mutable_msg<msg_initial_message> >( *this, "bye" );
	}

private :
	std::string m_received;
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop(
			[]( so_5::coop_t & coop ) {
				coop.make_agent< agent_with_limit_t >();
			} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init,
					[](so_5::environment_params_t & params) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					} );
			},
			5 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

