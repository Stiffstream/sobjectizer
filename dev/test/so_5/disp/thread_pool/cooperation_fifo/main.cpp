/*
 * A simple test for thread_pool dispatcher.
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
#include <so_5/h/spinlocks.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/benchmark_helpers.hpp>

#include "../for_each_lock_factory.hpp"

namespace tp_disp = so_5::disp::thread_pool;

typedef std::set< so_5::current_thread_id_t > thread_id_set_t;

class thread_id_collector_t
	{
	public :
		void lock()
		{
			m_lock.lock();
		}

		void unlock()
		{
			m_lock.unlock();
		}

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

typedef std::shared_ptr< thread_id_collector_t > thread_id_collector_ptr_t;

typedef std::vector< thread_id_collector_ptr_t > collector_container_t;

struct msg_shutdown : public so_5::signal_t {};

struct msg_hello : public so_5::signal_t {};

/*
 * There is a trick in working scheme for this agent.
 *
 * The first agent in cooperation will be blocked in so_evt_start()
 * on m_collector.add_current_thread() call because collector will
 * be locked before start of cooperation registration.
 * Collector will be unlocked after return from register_coop().
 * At this moment there must be demands for so_evt_start for
 * all cooperation agents in the same agent_queue.
 *
 * During processing of so_evt_start() new demands (for msg_hello)
 * will be placed to the same agent_queue. And this queue will be
 * processed on the same working thread because of big value
 * of max_demands_at_once parameter.
 */
class a_test_t : public so_5::agent_t
{
	public:
		a_test_t(
			so_5::environment_t & env,
			thread_id_collector_t & collector,
			const so_5::mbox_t & shutdowner_mbox )
			:	so_5::agent_t( env )
			,	m_collector( collector )
		{
			so_subscribe_self().event< msg_hello >(
				[shutdowner_mbox]() {
					shutdowner_mbox->deliver_signal< msg_shutdown >();
				} );
		}

		void
		so_evt_start()
		{
			m_collector.add_current_thread();

			so_direct_mbox()->deliver_signal< msg_hello >();
		}

	private :
		thread_id_collector_t & m_collector;
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

		virtual void
		so_define_agent()
		{
			so_subscribe_self().event< msg_shutdown >( [=] {
					--m_working_agents;
					if( !m_working_agents )
						so_environment().stop();
				} );
		}

	private :
		std::size_t m_working_agents;
};

const std::size_t cooperation_count = 1024; // 1000;
const std::size_t cooperation_size = 128; // 100;
const std::size_t thread_count = 8;

collector_container_t
create_collectors()
{
	collector_container_t collectors;
	collectors.reserve( cooperation_count );
	for( std::size_t i = 0; i != cooperation_count; ++i )
		collectors.emplace_back( std::make_shared< thread_id_collector_t >() );

	return collectors;
}

void
run_sobjectizer(
	tp_disp::queue_traits::lock_factory_t factory,
	collector_container_t & collectors )
{
	duration_meter_t duration( "running of test cooperations" );

	so_5::launch(
		[&]( so_5::environment_t & env )
		{
			so_5::mbox_t shutdowner_mbox;
			{
				auto c = env.create_coop( "shutdowner" );
				auto a = c->add_agent( new a_shutdowner_t( env,
						cooperation_count * cooperation_size ) );
				shutdowner_mbox = a->so_direct_mbox();
				env.register_coop( std::move( c ) );
			}

			tp_disp::bind_params_t params;
			params.max_demands_at_once( 1024 );
			for( std::size_t i = 0; i != cooperation_count; ++i )
			{
				// Lock collector for that cooperation until
				// register_coop finished.
				// It guarantees that the first cooperation agent
				// will be blocked in so_evt_start. And demands for
				// other agents will be placed into the same demands queue.

				std::lock_guard< thread_id_collector_t > collector_lock(
						*(collectors[ i ]) );

				std::ostringstream ss;
				ss << "coop_" << i;

				auto c = env.create_coop( ss.str(),
						tp_disp::create_disp_binder(
								"thread_pool", params ) );
				for( std::size_t a = 0; a != cooperation_size; ++a )
				{
					c->add_agent(
							new a_test_t(
									env,
									*(collectors[ i ]),
									shutdowner_mbox ) );
				}
				env.register_coop( std::move( c ) );
			}
		},
		[&]( so_5::environment_params_t & params )
		{
			using namespace tp_disp;
			params.add_named_dispatcher(
					"thread_pool",
					create_disp( disp_params_t{}
							.thread_count( thread_count )
							.set_queue_params( queue_traits::queue_params_t{}
								.lock_factory( factory ) ) )
			);
		} );
}

void
analyze_results( const collector_container_t & collectors )
{
	thread_id_set_t all_threads;

	for( auto & c : collectors )
		if( 1 != c->set_size() )
		{
			std::ostringstream ss;
			ss << "there is a set with size: " << c->set_size();
			throw std::runtime_error( ss.str() );
		}
		else
			all_threads.insert( c->query_set().begin(), c->query_set().end() );

	std::cout << "all_threads size: " << all_threads.size() << std::endl;
}

void
run_and_check(
	tp_disp::queue_traits::lock_factory_t factory )
{
	auto collectors = create_collectors();

	run_sobjectizer( factory, collectors );

	analyze_results( collectors );
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
				240,
				"cooperation_fifo test" );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

