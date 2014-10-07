/*
 * A simple implementation of demo of dining philosophers
 * problem. See description of this problem in Wikipedia:
 * http://en.wikipedia.org/wiki/Dining_philosophers_problem
 */

#include <iostream>
#include <iterator>
#include <numeric>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <tuple>

#include <so_5/all.hpp>

struct msg_take : public so_5::rt::message_t
{
	const so_5::rt::mbox_ref_t m_who;

	msg_take( so_5::rt::mbox_ref_t who ) : m_who( std::move( who ) ) {}
};

struct msg_busy : public so_5::rt::signal_t {};

struct msg_taken : public so_5::rt::signal_t {};

struct msg_put : public so_5::rt::signal_t {};

class a_fork_t : public so_5::rt::agent_t
{
public :
	a_fork_t( so_5::rt::environment_t & env )
		:	so_5::rt::agent_t( env )
	{}

	virtual void
	so_define_agent() override
	{
		so_change_state( st_free );

		so_subscribe( so_direct_mbox() ).in( st_free )
			.event( [this]( const msg_take & evt )
				{
					so_change_state( st_taken );
					evt.m_who->deliver_signal< msg_taken >();
				} );

		so_subscribe( so_direct_mbox() ).in( st_taken )
			.event( []( const msg_take & evt )
				{
					evt.m_who->deliver_signal< msg_busy >();
				} )
			.event( so_5::signal< msg_put >,
				[this]()
				{
					so_change_state( st_free );
				} );
	}

private :
	const so_5::rt::state_t st_free = so_make_state( "free" );
	const so_5::rt::state_t st_taken = so_make_state( "taken" );
};

class a_philosopher_t : public so_5::rt::agent_t
{
	struct msg_stop_thinking : public so_5::rt::signal_t {};
	struct msg_stop_eating : public so_5::rt::signal_t {};

public :
	a_philosopher_t(
		so_5::rt::environment_t & env,
		std::string name,
		so_5::rt::mbox_ref_t left_fork,
		so_5::rt::mbox_ref_t right_fork )
		:	so_5::rt::agent_t( env )
		,	m_name( std::move( name ) )
		,	m_left_fork( std::move( left_fork ) )
		,	m_right_fork( std::move( right_fork ) )
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe( so_direct_mbox() ).in( st_thinking )
			.event( so_5::signal< msg_stop_thinking >,
				[this]()
				{
					show_msg( "become hungry, try to take left fork" );
					so_change_state( st_wait_left );
					m_left_fork->deliver_message( new msg_take( so_direct_mbox() ) );
				} );

		so_subscribe( so_direct_mbox() ).in( st_wait_left )
			.event( so_5::signal< msg_taken >,
				[this]()
				{
					show_msg( "left fork taken, try to take right fork" );
					so_change_state( st_wait_right );
					m_right_fork->deliver_message( new msg_take( so_direct_mbox() ) );
				} )
			.event( so_5::signal< msg_busy >,
				[this]()
				{
					show_msg( "left fork is busy, return to thinking" );
					return_to_thinking();
				} );

		so_subscribe( so_direct_mbox() ).in( st_wait_right )
			.event( so_5::signal< msg_taken >,
				[this]()
				{
					show_msg( "right fork taken, start eating" );
					so_change_state( st_eating );
					so_environment().single_timer< msg_stop_eating >(
						so_direct_mbox(), random_pause() );
				} )
			.event( so_5::signal< msg_busy >,
				[this]()
				{
					show_msg( "right fork is busy, put left fork, return to thinking" );
					m_left_fork->deliver_signal< msg_put >();
					return_to_thinking();
				} );

		so_subscribe( so_direct_mbox() ).in( st_eating )
			.event( so_5::signal< msg_stop_eating >,
				[this]()
				{
					show_msg( "stop eating, put right fork, put left fork, "
						"return to thinking" );
					m_right_fork->deliver_signal< msg_put >();
					m_left_fork->deliver_signal< msg_put >();
					return_to_thinking();
				} );
	}

	virtual void
	so_evt_start() override
	{
		return_to_thinking();
	}

private :
	const so_5::rt::state_t st_thinking = so_make_state( "thinking" );
	const so_5::rt::state_t st_wait_left = so_make_state( "wait_left" );
	const so_5::rt::state_t st_wait_right = so_make_state( "wait_right" );
	const so_5::rt::state_t st_eating = so_make_state( "eating" );

	const std::string m_name;

	const so_5::rt::mbox_ref_t m_left_fork;
	const so_5::rt::mbox_ref_t m_right_fork;

	void
	show_msg( const std::string & msg ) const
	{
		std::cout << "[" << m_name << "] " << msg << std::endl;
	}

	void
	return_to_thinking()
	{
		so_change_state( st_thinking );
		so_environment().single_timer< msg_stop_thinking >(
			so_direct_mbox(), random_pause() );
	}

	static std::chrono::milliseconds
	random_pause()
	{
		return std::chrono::milliseconds( 250 + (std::rand() % 250) );
	}
};

void
init( so_5::rt::environment_t & env )
{
	const std::size_t count = 5;

	auto coop = env.create_coop( "dining_philosophers" );

	std::vector< so_5::rt::agent_t * > forks( count, nullptr );
	for( std::size_t i = 0; i != count; ++i )
		forks[ i ] = coop->add_agent( new a_fork_t( env ) );

	for( std::size_t i = 0; i != count; ++i )
		coop->add_agent(
			new a_philosopher_t( env,
				std::to_string( i ),
				forks[ i ]->so_direct_mbox(),
				forks[ (i + 1) % count ]->so_direct_mbox() ) );

	struct msg_shutdown : public so_5::rt::signal_t {};
	auto shutdown_mbox = env.create_local_mbox();
	coop->define_agent()
		.event( shutdown_mbox, so_5::signal< msg_shutdown >,
			[&env]() { env.stop(); } );

	env.single_timer< msg_shutdown >( shutdown_mbox, std::chrono::seconds(10) );

	env.register_coop( std::move( coop ) );
}

int
main( int argc, char ** argv )
{
	try
	{
		so_5::launch( init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

