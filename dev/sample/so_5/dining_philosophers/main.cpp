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
		this >>= st_free;

		st_free.event( [=]( const msg_take & evt )
				{
					this >>= st_taken;
					so_5::send< msg_taken >( evt.m_who );
				} );

		st_taken.event( []( const msg_take & evt )
				{
					so_5::send< msg_busy >( evt.m_who );
				} )
			.event< msg_put >( [=] { this >>= st_free; } );
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
		st_thinking.event< msg_stop_thinking >( [=] {
				show_msg( "become hungry, try to take left fork" );
				this >>= st_wait_left;
				so_5::send< msg_take >( m_left_fork, so_direct_mbox() );
			} );

		st_wait_left.event< msg_taken >( [=] {
				show_msg( "left fork taken, try to take right fork" );
				this >>= st_wait_right;
				so_5::send< msg_take >( m_right_fork, so_direct_mbox() );
			} )
			.event< msg_busy >( [=] {
				show_msg( "left fork is busy, return to thinking" );
				think();
			} );

		st_wait_right.event< msg_taken >( [=] {
				show_msg( "right fork taken, start eating" );
				this >>= st_eating;
				so_5::send_delayed_to_agent< msg_stop_eating >( *this, pause() );
			} )
			.event< msg_busy >( [=] {
				show_msg( "right fork is busy, put left fork, return to thinking" );
				so_5::send< msg_put >( m_left_fork );
				think();
			} );

		st_eating.event< msg_stop_eating >( [=] {
				show_msg( "stop eating, put forks, return to thinking" );
				so_5::send< msg_put >( m_right_fork );
				so_5::send< msg_put >( m_left_fork );
				think();
			} );
	}

	virtual void
	so_evt_start() override
	{
		think();
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
	think()
	{
		this >>= st_thinking;
		so_5::send_delayed_to_agent< msg_stop_thinking >( *this, pause() );
	}

	static std::chrono::milliseconds
	pause()
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

	env.register_coop( std::move( coop ) );

	std::this_thread::sleep_for( std::chrono::seconds(20) );
	env.stop();
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

