/*
 * A test for thread_pool dispatcher when a single binder with
 * cooperative_fifo is used for several coops.
 */

#include <iostream>
#include <set>
#include <vector>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <sstream>

#include <so_5/all.hpp>
#include <so_5/spinlocks.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/benchmark_helpers.hpp>

#include "../for_each_lock_factory.hpp"

namespace tp_disp = so_5::disp::thread_pool;

typedef std::set< so_5::current_thread_id_t > thread_id_set_t;

class thread_id_collector_t
	{
	public :
		void add_current_thread()
		{
			std::lock_guard< so_5::default_spinlock_t > l( m_lock );

			m_set.insert( so_5::query_current_thread_id() );
		}

		std::size_t set_size() const
		{
			return m_set.size();
		}

		const thread_id_set_t &
		query_set() const
		{
			return m_set;
		}

	private :
		so_5::default_spinlock_t m_lock;
		thread_id_set_t m_set;
	};

struct msg_shutdown : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t(
			so_5::environment_t & env,
			thread_id_collector_t & collector,
			const so_5::mbox_t & shutdowner_mbox )
			:	so_5::agent_t( env )
			,	m_collector( collector )
			,	m_shutdowner_mbox( shutdowner_mbox )
		{
		}

		void
		so_evt_start() override
		{
			// Block the current thread for some time.
			// Because of that so_evt_start for an agent from a different
			// coop has to be started on a separate thread.
			std::this_thread::sleep_for( std::chrono::milliseconds{ 250 } );

			m_collector.add_current_thread();

			so_5::send< msg_shutdown >( m_shutdowner_mbox );
		}

	private :
		thread_id_collector_t & m_collector;
		const so_5::mbox_t m_shutdowner_mbox;
};

class a_shutdowner_t : public so_5::agent_t
{
	public :
		a_shutdowner_t(
			so_5::environment_t & env,
			std::size_t working_agents )
			:	so_5::agent_t( env )
			,	m_working_agents( working_agents )
		{}

		void
		so_define_agent() override
		{
			so_subscribe_self().event( [this](mhood_t< msg_shutdown >) {
					--m_working_agents;
					if( !m_working_agents )
						so_environment().stop();
				} );
		}

	private :
		std::size_t m_working_agents;
};

const std::size_t cooperation_count = 4;

void
run_sobjectizer(
	tp_disp::queue_traits::lock_factory_t factory,
	thread_id_collector_t & collector )
{
	duration_meter_t duration( "running of test cooperations" );

	so_5::launch(
		[&]( so_5::environment_t & env )
		{
			so_5::mbox_t shutdowner_mbox;
			{
				auto c = env.make_coop();
				auto a = c->make_agent< a_shutdowner_t >( cooperation_count );
				shutdowner_mbox = a->so_direct_mbox();
				env.register_coop( std::move( c ) );
			}

			auto disp = tp_disp::make_dispatcher(
					env,
					"thread_pool",
					tp_disp::disp_params_t{}
							.thread_count( cooperation_count )
							.set_queue_params( tp_disp::queue_traits::queue_params_t{}
									.lock_factory( factory ) ) );

			auto params = tp_disp::bind_params_t{}
					.max_demands_at_once( 1024 )
					.fifo( tp_disp::fifo_t::cooperation );
			auto the_same_binder = disp.binder( params );

			for( std::size_t i = 0; i != cooperation_count; ++i )
			{
				auto c = env.make_coop( the_same_binder );
				c->make_agent< a_test_t >( collector, shutdowner_mbox );
				env.register_coop( std::move( c ) );
			}
		} );
}

void
analyze_results( const thread_id_collector_t & collector )
{
	if( cooperation_count != collector.set_size() )
	{
		throw std::runtime_error{
			"there is a set with size: " + std::to_string( collector.set_size() )
		};
	}
}

void
run_and_check(
	tp_disp::queue_traits::lock_factory_t factory )
{
	thread_id_collector_t collector;

	run_sobjectizer( factory, collector );

	analyze_results( collector );
}

int
main()
{
	try
	{
		for_each_lock_factory( []( tp_disp::queue_traits::lock_factory_t factory ) {
			run_with_time_limit(
				[&]()
				{
					run_and_check( factory );
				},
				240 );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

