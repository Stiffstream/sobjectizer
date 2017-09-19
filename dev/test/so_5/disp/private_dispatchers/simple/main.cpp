/*
 * A simple test for private dispatchers.
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

struct msg_start : public so_5::signal_t {};

struct msg_hello : public so_5::message_t
{
	std::string m_who;

	msg_hello( std::string who ) : m_who( std::move( who ) )
	{}
};

class a_collector_t : public so_5::agent_t
{
private :
	const so_5::mbox_t m_start_mbox;
	unsigned int m_remaining;	

public :
	a_collector_t(
		so_5::environment_t & env,
		const so_5::mbox_t & start_mbox,
		unsigned int messages_to_receive )
		:	so_5::agent_t( env )
		,	m_start_mbox( start_mbox )
		,	m_remaining( messages_to_receive )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state().event( [this]( const msg_hello & msg ) {
				std::cout << "received: " << msg.m_who << std::endl;
				if( 0 == (--m_remaining) )
					so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< msg_start >( m_start_mbox );
	}
};

std::string
make_hello_string( const char * who )
{
	std::ostringstream ss;
	ss << who << " from thread [" << so_5::query_current_thread_id() << "]";
	return ss.str();
}

void
init( so_5::environment_t & env )
{
	auto one_thread = so_5::disp::one_thread::create_private_disp( env );
	auto active_obj = so_5::disp::active_obj::create_private_disp( env );
	auto active_group = so_5::disp::active_group::create_private_disp( env );
	auto thread_pool = so_5::disp::thread_pool::create_private_disp( env, 3 );
	auto adv_thread_pool = so_5::disp::adv_thread_pool::create_private_disp( env, 10 );

	auto start_mbox = env.create_mbox( "start" );
	auto coop = env.create_coop( so_5::autoname );
	
	auto collector = coop->make_agent< a_collector_t >( start_mbox, 9u + 10u );

	coop->define_agent( one_thread->binder() )
		.event< msg_start >( start_mbox,
				[collector]() {
					so_5::send_to_agent< msg_hello >( *collector,
							make_hello_string( "one_thread" ) );
				} );

	coop->define_agent( active_obj->binder() )
		.event< msg_start >( start_mbox,
				[collector]() {
					so_5::send_to_agent< msg_hello >( *collector,
							make_hello_string( "active_obj-1" ) );
				} );

	coop->define_agent( active_obj->binder() )
		.event< msg_start >( start_mbox,
				[collector]() {
					so_5::send_to_agent< msg_hello >( *collector,
							make_hello_string( "active_obj-2" ) );
				} );

	coop->define_agent( active_group->binder( "agOne") )
		.event< msg_start >( start_mbox,
				[collector]() {
					so_5::send_to_agent< msg_hello >( *collector,
							make_hello_string( "active_group-1" ) );
				} );

	coop->define_agent( active_group->binder( "agTwo") )
		.event< msg_start >( start_mbox,
				[collector]() {
					so_5::send_to_agent< msg_hello >( *collector,
							make_hello_string( "active_group-2-1" ) );
				} );

	coop->define_agent( active_group->binder( "agTwo") )
		.event< msg_start >( start_mbox,
				[collector]() {
					so_5::send_to_agent< msg_hello >( *collector,
							make_hello_string( "active_group-2-2" ) );
				} );

	const auto tp_params = so_5::disp::thread_pool::bind_params_t{}
			.fifo( so_5::disp::thread_pool::fifo_t::individual );

	coop->define_agent( thread_pool->binder( tp_params ) )
		.event< msg_start >( start_mbox,
				[collector]() {
					std::this_thread::sleep_for( std::chrono::seconds(2) );
					so_5::send_to_agent< msg_hello >( *collector,
							make_hello_string( "thread_pool-1" ) );
				} );
	coop->define_agent( thread_pool->binder( tp_params ) )
		.event< msg_start >( start_mbox,
				[collector]() {
					std::this_thread::sleep_for( std::chrono::seconds(2) );
					so_5::send_to_agent< msg_hello >( *collector,
							make_hello_string( "thread_pool-2" ) );
				} );
	coop->define_agent( thread_pool->binder( tp_params ) )
		.event< msg_start >( start_mbox,
				[collector]() {
					std::this_thread::sleep_for( std::chrono::seconds(2) );
					so_5::send_to_agent< msg_hello >( *collector,
							make_hello_string( "thread_pool-3" ) );
				} );

	const auto atp_params = so_5::disp::adv_thread_pool::bind_params_t{}
			.fifo( so_5::disp::adv_thread_pool::fifo_t::individual );

	auto atp_agent = coop->define_agent( adv_thread_pool->binder( atp_params ) );
	atp_agent
		.event< msg_start >( start_mbox,
				[atp_agent]() {
					for( int i = 0; i != 10; ++i )
						so_5::send< msg_start >( atp_agent.direct_mbox() );
				} )
		.event< msg_start >( atp_agent,
				[collector]() {
					std::this_thread::sleep_for( std::chrono::seconds(1) );
					so_5::send_to_agent< msg_hello >( *collector,
							make_hello_string( "adv_thread_pool" ) );
				},
				so_5::thread_safe );

	env.register_coop( std::move( coop ) );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			20,
			"simple private dispatchers test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

