/*
 * A simple implementation of arbiter-based solution dining philosophers
 * problem. See description of this problem in Wikipedia:
 * http://en.wikipedia.org/wiki/Dining_philosophers_problem
 */

#include <iostream>
#include <iterator>
#include <numeric>
#include <cstdlib>
#include <vector>
#include <mutex>

#include <so_5/all.hpp>

#define ENABLE_TRACE
#define USE_SELF_CHECK

#if defined( ENABLE_TRACE )
	std::mutex g_trace_mutex;

	class locker_t
	{
	public :
		locker_t( std::mutex & m ) : m_lock( m ) {}
		locker_t( locker_t && o ) : m_lock( std::move( o.m_lock ) ) {}

		operator bool() const { return true; }

	private :
		std::unique_lock< std::mutex > m_lock;
	};

	#define TRACE_MESSAGE() \
	if( auto l = locker_t{ g_trace_mutex } ) std::cout

#else
	#define TRACE_MESSAGE() \
	if( false ) std::cout

#endif

struct msg_start_eating_request : public so_5::rt::message_t
{
	std::size_t m_philosopher;

	msg_start_eating_request( std::size_t philosopher )
		:	m_philosopher( philosopher )
	{}
};

struct msg_start_eating : public so_5::rt::signal_t {};

struct msg_eating_finished : public so_5::rt::message_t
{
	std::size_t m_philosopher;

	msg_eating_finished( std::size_t philosopher )
		:	m_philosopher( philosopher )
	{}
};

struct fork_state_t
{
	bool m_in_use = false;
	bool m_someone_is_waiting = false;
};

class a_arbiter_t : public so_5::rt::agent_t
{
	struct msg_shutdown : public so_5::rt::signal_t {};

public :
	a_arbiter_t(
		so_5::rt::environment_t & env,
		std::size_t philosophers_count,
		std::chrono::seconds test_duration )
		:	so_5::rt::agent_t( env )
		,	m_philosophers_count( philosophers_count )
		,	m_test_duration( test_duration )
		,	m_forks( philosophers_count, fork_state_t() )
	{
		m_philosophers.reserve( philosophers_count );

		self_check__init();
	}

	void
	add_philosopher(
		const so_5::rt::mbox_ref_t & mbox )
	{
		m_philosophers.emplace_back( mbox );
	}

	virtual void
	so_define_agent() override
	{
		so_subscribe( so_direct_mbox() )
			.event( so_5::signal< msg_shutdown >,
					[this]() { so_environment().stop(); } );

		so_subscribe( so_direct_mbox() )
			.event( &a_arbiter_t::evt_start_eating_request );

		so_subscribe( so_direct_mbox() )
			.event( &a_arbiter_t::evt_eating_finished );
	}

	virtual void
	so_evt_start()
	{
		so_environment().single_timer< msg_shutdown >(
				so_direct_mbox(),
				m_test_duration );
	}

	void
	evt_start_eating_request( const msg_start_eating_request & evt )
	{
		try_allow_philosopher_to_eat( evt.m_philosopher );

		self_check__ensure_invariants();
	}

	void
	evt_eating_finished( const msg_eating_finished & evt )
	{
		self_check__philosopher_is_thinking( evt.m_philosopher );

		auto & left_fork = m_forks[ evt.m_philosopher ];
		left_fork.m_in_use = false;

		if( left_fork.m_someone_is_waiting )
		{
			// Left neighbor is waiting this fork as his right fork.
			left_fork.m_someone_is_waiting = false;
			left_fork.m_in_use = true;

			enable_eating_for_philosopher( left_neighbor( evt.m_philosopher ) );
		}

		const auto right_fork_index = right_neighbor( evt.m_philosopher );
		auto & right_fork = m_forks[ right_fork_index ];
		right_fork.m_in_use = false;

		if( right_fork.m_someone_is_waiting )
		{
			// Right neighbor is waiting this fork as his left fork.
			right_fork.m_someone_is_waiting = false;
			try_allow_philosopher_to_eat( right_fork_index );
		}

		self_check__ensure_invariants();
	}

private :
	const std::size_t m_philosophers_count;

	const std::chrono::seconds m_test_duration;

	std::vector< fork_state_t > m_forks;

	std::vector< so_5::rt::mbox_ref_t > m_philosophers;

	void
	try_allow_philosopher_to_eat( std::size_t philosopher )
	{
		auto & left_fork = m_forks[ philosopher ];
		if( left_fork.m_in_use )
		{
			if( left_fork.m_someone_is_waiting )
			{
				std::cerr << "fork(" << philosopher
					<< "), left for philosopher(" << philosopher
					<< "): is in use and someone is waiting for it"
					<< std::endl;
				std::abort();
			}
			else
			{
				// Just mark that there is a waiting philosopher for this fork.
				// No more can be done now.
				left_fork.m_someone_is_waiting = true;

				self_check__philosopher_is_waiting( philosopher );
			}
		}
		else
		{
			// This philosopher acquired his left fork.
			left_fork.m_in_use = true;

			// Checking availability of this right fork.
			const auto right_fork_index = right_neighbor( philosopher );
			auto & right_fork = m_forks[ right_fork_index ];
			if( right_fork.m_in_use )
			{
				if( right_fork.m_someone_is_waiting )
				{
					std::cerr << "fork(" << right_fork_index
						<< "), right for philosopher(" << philosopher
						<< "): is in use and someone is waiting for it"
						<< std::endl;
					std::abort();
				}
				else
				{
					// Just mark that there is a waiting philosopher for this fork.
					// No more can be done now.
					right_fork.m_someone_is_waiting = true;

					self_check__philosopher_is_waiting( philosopher );
				}
			}
			else
			{
				// This philosopher acquired his right fork.
				right_fork.m_in_use = true;

				// Philosopher can start eating.
				enable_eating_for_philosopher( philosopher );
			}
		}
	}

	std::size_t
	left_neighbor( std::size_t index ) const
	{
		if( index ) return index - 1;
		else return m_philosophers_count - 1;
	}

	std::size_t
	right_neighbor( std::size_t index ) const
	{
		return (index + 1) % m_philosophers_count;
	}

	void
	enable_eating_for_philosopher( std::size_t philosopher )
	{
		self_check__philosopher_is_eating( philosopher );

		m_philosophers[ philosopher ]->deliver_signal< msg_start_eating >();
	}

#if !defined( USE_SELF_CHECK )
	void
	self_check__init() {}

	void
	self_check__philosopher_is_eating( std::size_t ) {}

	void
	self_check__philosopher_is_thinking( std::size_t ) {}

	void
	self_check__philosopher_is_waiting( std::size_t ) {}

	void
	self_check__ensure_invariants() {}
#else
	enum class philosopher_state_t
	{
		thinking,
		waiting,
		eating
	};

	std::vector< philosopher_state_t > m_philosopher_states;

	void
	self_check__init()
	{
		m_philosopher_states.resize(
				m_philosophers_count,
				philosopher_state_t::thinking );
	}

	void
	self_check__philosopher_is_eating( std::size_t index )
	{
		m_philosopher_states[ index ] = philosopher_state_t::eating;
	}

	void
	self_check__philosopher_is_thinking( std::size_t index )
	{
		m_philosopher_states[ index ] = philosopher_state_t::thinking;
	}

	void
	self_check__philosopher_is_waiting( std::size_t index )
	{
		m_philosopher_states[ index ] = philosopher_state_t::waiting;
	}

	void
	self_check__ensure_invariants() const
	{
		for( std::size_t i = 0; i != m_philosophers_count; ++i )
		{
			auto p = m_philosopher_states[ i ];

			if( philosopher_state_t::eating == p )
			{
				// Threre must not be two consequtive eating philosophers.
				auto r1 = m_philosopher_states[ right_neighbor( i ) ];
				if( p == r1 )
				{
					std::cerr << "INVARIANT VIOLATED: two 'eating' in a row "
							"staring from #" << i << std::endl;
					self_check__dump();
					std::abort();
				}
			}
		}
	}

	void
	self_check__dump() const
	{
		for( auto f : m_forks )
			std::cerr << (f.m_in_use ? "B" : "F" )
					<< (f.m_someone_is_waiting ? "(1)" : "(0)")
					<< ":";
		std::cerr << std::endl;

		for( auto s : m_philosopher_states )
			std::cerr << (philosopher_state_t::thinking == s ?
					"T" : (philosopher_state_t::waiting == s ?
						"W" : "E")) << ":";
		std::cerr << std::endl;
	}
#endif

};

class a_philosopher_t : public so_5::rt::agent_t
{
	struct msg_start_thinking : public so_5::rt::signal_t {};

public :
	a_philosopher_t(
		so_5::rt::environment_t & env,
		std::size_t index,
		const so_5::rt::mbox_ref_t & arbiter_mbox )
		:	so_5::rt::agent_t( env )
		,	m_index( index )
		,	m_arbiter_mbox( arbiter_mbox )
	{}

	virtual void
	so_define_agent()
	{
		so_subscribe( so_direct_mbox() )
			.event( so_5::signal< msg_start_thinking >,
					&a_philosopher_t::evt_start_thinking );

		so_subscribe( so_direct_mbox() )
			.event( so_5::signal< msg_start_eating >,
					&a_philosopher_t::evt_start_eating );
	}

	virtual void
	so_evt_start()
	{
		initiate_thinking();
	}

	void
	evt_start_thinking()
	{
		TRACE_MESSAGE() << "[" << m_index << "] Started thinking" << std::endl;

		think();

		TRACE_MESSAGE() << "[" << m_index << "] Stopped thinking" << std::endl;

		send_start_eating_request();
	}

	void
	evt_start_eating()
	{
		TRACE_MESSAGE() << "[" << m_index << "] Started eating" << std::endl;

		eat();

		TRACE_MESSAGE() << "[" << m_index << "] Stopped eating" << std::endl;

		m_arbiter_mbox->deliver_message(
				new msg_eating_finished( m_index ) );

		initiate_thinking();
	}

	void
	evt_stop_eating()
	{
		TRACE_MESSAGE() << "[" << m_index << "] Stopped eating" << std::endl;

		send_start_eating_request();
	}

protected :
	virtual void
	think()
	{
		const auto p = std::chrono::milliseconds( std::rand() % 250 );

		TRACE_MESSAGE() << "[" << m_index << "] Dummy thinking for "
				<< p.count() << "ms" << std::endl;

		std::this_thread::sleep_for( p );
	}

	virtual void
	eat()
	{
		const auto p = std::chrono::milliseconds( std::rand() % 250 );

		TRACE_MESSAGE() << "[" << m_index << "] Dummy eating for "
				<< p.count() << "ms" << std::endl;

		std::this_thread::sleep_for( p );
	}

private :
	const std::size_t m_index;

	const so_5::rt::mbox_ref_t m_arbiter_mbox;

	void
	initiate_thinking()
	{
		so_direct_mbox()->deliver_signal< msg_start_thinking >();
	}

	void
	send_start_eating_request()
	{
		TRACE_MESSAGE() << "[" << m_index << "] Waiting" << std::endl;

		m_arbiter_mbox->deliver_message(
				new msg_start_eating_request( m_index ) );
	}
};

void
init( so_5::rt::environment_t & env )
{
	const std::size_t philosophers_count = 5;

	auto coop = env.create_coop( "dining_philosophers_with_arbiter" );

	auto arbiter = coop->add_agent(
			new a_arbiter_t(
				env,
				philosophers_count,
				std::chrono::seconds(1) ) );

	for( std::size_t i = 0; i != philosophers_count; ++i )
	{
		auto p = coop->add_agent(
				new a_philosopher_t(
					env,
					i,
					arbiter->so_direct_mbox() ),
				so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

		arbiter->add_philosopher( p->so_direct_mbox() );
	}

	env.register_coop( std::move( coop ) );
}

int
main()
{
	try
	{
		so_5::launch(
				init,
				[]( so_5::rt::environment_params_t & p ) {
					p.add_named_dispatcher( "active_obj",
							so_5::disp::active_obj::create_disp() );
				} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

