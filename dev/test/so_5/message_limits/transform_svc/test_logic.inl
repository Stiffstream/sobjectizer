/*
 * A simple test for message limits (transforming the service request).
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_hello : public so_5::rt::message_t
{
	std::string m_text;

	msg_hello( std::string text )
		:	m_text( std::move( text ) )
	{}
};

struct msg_hello_overlimit : public so_5::rt::message_t
{
	std::string m_desc;

	msg_hello_overlimit( std::string desc )
		:	m_desc( std::move( desc ) )
	{}
};

class a_test_t : public so_5::rt::agent_t
{
public :
	a_test_t(
		so_5::rt::environment_t & env )
		:	so_5::rt::agent_t( env
				+ limit_then_transform( 1,
					[&]( const msg_hello & msg ) {
						return make_transformed< msg_hello_overlimit >(
								m_working_mbox, "<=" + msg.m_text + "=>" );
					} )
				+ limit_then_drop< msg_hello_overlimit >( 1 )
				+ limit_then_drop< msg_finish >( 1 )
			 )
	{}

	void
	set_working_mbox( const so_5::rt::mbox_t & mbox )
	{
		m_working_mbox = mbox;
	}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event( m_working_mbox, [&]( const msg_hello & msg ) -> std::string {
					return "[" + msg.m_text + "]";
				} )
			.event( m_working_mbox, [&]( const msg_hello_overlimit & ) {
					throw std::runtime_error( "msg_hello_overlimit received!" );
				} )
			.event< msg_finish >( m_working_mbox, [&] {
					const std::string expected = "[hello]";
					const std::string actual = m_r1.get();

					if( expected != actual )
						throw std::runtime_error( expected + " != " + actual );
					else
						so_deregister_agent_coop_normally();
				} );
	}

	virtual void
	so_evt_start() override
	{
		m_r1 = m_working_mbox->get_one< std::string >()
				.make_async< msg_hello >( "hello" );

		try
		{
			auto s = m_working_mbox->get_one< std::string >()
					.wait_forever().make_sync_get< msg_hello >( "hello2" );

			throw std::runtime_error( "An exception expected!" );
		}
		catch( const so_5::exception_t & x )
		{
			if( so_5::rc_svc_request_cannot_be_transfomred_on_overlimit !=
					x.error_code() )
				throw;
		}

		so_5::send< msg_finish >( m_working_mbox );
	}

private :
	struct msg_finish : public so_5::rt::signal_t {};

	so_5::rt::mbox_t m_working_mbox;

	std::future< std::string > m_r1;
};

void
do_test(
	const std::string & test_name,
	std::function< void(a_test_t &) > test_tuner )
{
	try
	{
		run_with_time_limit(
			[test_tuner]()
			{
				so_5::launch(
						[test_tuner]( so_5::rt::environment_t & env )
						{
							auto coop = env.create_coop( so_5::autoname );
							auto agent = coop->make_agent< a_test_t >();

							test_tuner( *agent );

							env.register_coop( std::move( coop ) );
						} );
			},
			20,
			test_name );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		std::abort();
	}
}

