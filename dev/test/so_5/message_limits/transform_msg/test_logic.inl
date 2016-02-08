/*
 * A simple test for message limits (transforming the message).
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

struct msg_double_overlimit : public so_5::rt::signal_t {};

struct msg_triple_overlimit : public so_5::rt::signal_t {};

struct msg_final_overlimit : public so_5::rt::message_t
{
	std::string m_text;

	msg_final_overlimit( std::string text )
		:	m_text( std::move( text ) )
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
				+ limit_then_transform( 1,
					[&]( const msg_hello_overlimit & ) {
						return make_transformed< msg_double_overlimit >(
								m_working_mbox );
					} )
				+ limit_then_transform< msg_double_overlimit >( 1,
					[&] {
						return make_transformed< msg_triple_overlimit >(
								m_working_mbox );
					} )
				+ limit_then_transform< msg_triple_overlimit >( 1,
					[&] {
						return make_transformed< msg_final_overlimit >(
								m_working_mbox, "done" );
					} )
				+ limit_then_drop< msg_final_overlimit >( 1 )
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
			.event( m_working_mbox, [&]( const msg_hello & msg ) {
					m_received += "[" + msg.m_text + "]";
				} )
			.event( m_working_mbox, [&]( const msg_hello_overlimit & msg ) {
					m_received += "[" + msg.m_desc + "]";
				} )
			.event< msg_double_overlimit >( m_working_mbox, [&] {
					m_received += "[<double>]";
				} )
			.event< msg_triple_overlimit >( m_working_mbox, [&] {
					m_received += "[<triple>]";
				} )
			.event( m_working_mbox, [&]( const msg_final_overlimit & msg ) {
					m_received += "[" + msg.m_text + "]";
				} )
			.event< msg_finish >( m_working_mbox, [&] {
					const std::string expected =
						"[hello][<=hello2=>][<double>][<triple>][done]";

					if( expected != m_received )
						throw std::runtime_error( expected + " != " + m_received );
					else
						so_deregister_agent_coop_normally();
				} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< msg_hello >( m_working_mbox, "hello" );
		so_5::send< msg_hello >( m_working_mbox, "hello2" );
		so_5::send< msg_hello >( m_working_mbox, "hello3" );
		so_5::send< msg_hello >( m_working_mbox, "hello4" );
		so_5::send< msg_hello >( m_working_mbox, "hello5" );
		so_5::send< msg_hello >( m_working_mbox, "hello6" );
		so_5::send< msg_finish >( m_working_mbox );
	}

private :
	struct msg_finish : public so_5::rt::signal_t {};

	so_5::rt::mbox_t m_working_mbox;

	std::string m_received;
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

