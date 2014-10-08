/*
 * A simple implementation of demo of dining philosophers
 * problem. See description of this problem in Wikipedia:
 * http://en.wikipedia.org/wiki/Dining_philosophers_problem
 *
 * Note: this is not a classical problem. In the classical
 * problem a philosopher must take the left fork first. Only
 * then he can take the right fork.
 * In this example a philosopher is trying to get both forks
 * at the same time.
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

struct msg_taken : public so_5::rt::message_t
{
	const so_5::rt::mbox_ref_t m_who;

	msg_taken( so_5::rt::mbox_ref_t who ) : m_who( std::move( who ) ) {}
};

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
		st_free.activate();

		st_free.handle( [this]( const msg_take & evt )
				{
					st_taken.activate();
					so_5::send< msg_taken >( evt.m_who, so_direct_mbox() );
				} );

		st_taken.handle( []( const msg_take & evt )
				{
					so_5::send< msg_busy >( evt.m_who );
				} )
			.handle< msg_put >( [this]() { st_free.activate(); } );
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
		st_thinking.handle< msg_stop_thinking >( [this]() {
				show_msg( "become hungry, try to take forks" );
				st_wait_any.activate();

				so_5::send< msg_take >( m_left_fork, so_direct_mbox() );
				so_5::send< msg_take >( m_right_fork, so_direct_mbox() );
			} );

		st_wait_any.handle( [this]( const msg_taken & evt ) {
				(evt.m_who == m_left_fork ?
				 	st_left_taken : st_right_taken).activate();

				m_first_taken = evt.m_who;
			} )
			.handle< msg_busy >( [this]() {
				st_wait_second.activate();
			} );

		st_left_taken.handle( [this]( const msg_taken & evt ) {
				start_eating( "forks taken (left, then right)" );
			} )
			.handle< msg_busy >( &a_philosopher_t::evt_second_busy );

		st_right_taken.handle( [this]( const msg_taken & evt ) {
				start_eating( "forks taken (right, then left)" );
			} )
			.handle< msg_busy >( &a_philosopher_t::evt_second_busy );

		st_wait_second.handle( [this]( const msg_taken & evt ) {
				so_5::send< msg_put >( evt.m_who );

				show_msg( "attempt to eat failed (one fork busy), "
					"return to thinking" );
				return_to_thinking();
			} )
			.handle< msg_busy >( [this]() {
				show_msg( "attempt to eat failed (both forks busy), "
					"return to thinking" );
				return_to_thinking();
			} );

		st_eating.handle< msg_stop_eating >( [this]() {
				show_msg( "stop eating, put forks, return to thinking" );

				so_5::send< msg_put >( m_right_fork );
				so_5::send< msg_put >( m_left_fork );
				return_to_thinking();
			} );
	}

	virtual void
	so_evt_start() override
	{
		return_to_thinking();
	}

	void
	evt_second_busy()
	{
		std::string first = (m_first_taken == m_left_fork ? "left" : "right");
		std::string second = (m_first_taken == m_left_fork ? "right" : "left");

		show_msg( first + " taken, but " + second + " busy, put " + first +
				", return to thinking" );
		so_5::send< msg_put >( m_first_taken );

		return_to_thinking();
	}

private :
	const so_5::rt::state_t st_thinking = so_make_state( "thinking" );
	const so_5::rt::state_t st_wait_any = so_make_state( "wait_any" );
	const so_5::rt::state_t st_wait_second = so_make_state( "wait_second" );
	const so_5::rt::state_t st_left_taken = so_make_state( "left_taken" );
	const so_5::rt::state_t st_right_taken = so_make_state( "right_taken" );
	const so_5::rt::state_t st_eating = so_make_state( "eating" );

	const std::string m_name;

	const so_5::rt::mbox_ref_t m_left_fork;
	const so_5::rt::mbox_ref_t m_right_fork;

	so_5::rt::mbox_ref_t m_first_taken;

	void
	show_msg( const std::string & msg ) const
	{
		std::cout << "[" << m_name << "] " << msg << std::endl;
	}

	void
	return_to_thinking()
	{
		show_msg( "start thinking" );
		st_thinking.activate();
		so_5::send_delayed_to_agent< msg_stop_thinking >( *this, random_pause() );
	}

	void
	start_eating( const std::string & msg )
	{
		show_msg( msg );
		st_eating.activate();
		so_5::send_delayed_to_agent< msg_stop_eating >( *this, random_pause() );
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

	so_5::send_delayed< msg_shutdown >(
			env, shutdown_mbox, std::chrono::seconds(10) );

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

