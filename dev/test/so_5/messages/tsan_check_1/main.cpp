/*
 * Test case for ThreadSanitizer.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <pthread.h>
#include <sys/resource.h>

namespace test
{

const int workers_count = 24;

struct take_me_task final : public so_5::message_t
	{
		const so_5::mbox_t m_reply_to;

		std::array< char, 256 > m_align_1;

		std::atomic< int > * m_counter;

		take_me_task(
			so_5::mbox_t reply_to,
			std::atomic< int > * counter )
			: m_reply_to{ std::move(reply_to) }
			, m_counter{ counter }
		{}

		~take_me_task()
		{
			(*m_counter) += 1;
		}
	};

struct your_task final : public so_5::signal_t {};

struct stop final : public so_5::signal_t {};
struct stop_ack final : public so_5::signal_t {};

alignas(128) std::atomic< int > g_normal_counter{ 0 };
alignas(128) std::atomic< int > g_failure_counter{ 0 };

class worker_t final : public so_5::agent_t
{
public :
	worker_t( context_t ctx, so_5::mbox_t manager )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_manager{ std::move(manager) }
		{}

	void
	so_define_agent() override
		{
			so_subscribe_self()
				.event( &worker_t::evt_your_task )
				.event( &worker_t::evt_stop )
				;
		}

	void
	so_evt_start() override
		{
#if 1
			 int policy;
			 sched_param param;
			 if( 0 != pthread_getschedparam( pthread_self(), &policy, &param ) )
				 throw std::runtime_error{ "pthread_getschedparam failed" };

			 policy = SCHED_BATCH;
			 param.sched_priority = sched_get_priority_min( policy );
			 if( 0 != pthread_setschedparam( pthread_self(), policy, &param ) )
				 throw std::runtime_error{ "pthread_setschedparam failed" };

			 if( 0 != setpriority( PRIO_PROCESS, 0, 2 ) )
				 throw std::runtime_error{ "setpriority failed" };
#endif

			so_5::send< so_5::mutable_msg< take_me_task > >( m_manager,
					so_direct_mbox(),
					std::addressof( g_failure_counter ) );
		}

private :
	const so_5::mbox_t m_manager;

	void
	evt_your_task( mhood_t< your_task > )
		{
//			std::this_thread::yield();

			so_5::send< so_5::mutable_msg< take_me_task > >( m_manager,
					so_direct_mbox(),
					std::addressof( g_failure_counter ) );
		}

	void
	evt_stop( mhood_t< stop > )
		{
			so_5::send< stop_ack >( m_manager );
		}
};

class manager_t final : public so_5::agent_t
{
public :
	manager_t( context_t ctx )
		:	so_5::agent_t{ std::move(ctx) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &manager_t::evt_take_me_task )
			.event( &manager_t::evt_stop_ack )
			;
	}

private :
	unsigned long m_tasks_sent{ 0 };
	int m_workers_stopped{ 0 };

	void
	evt_take_me_task( mutable_mhood_t< take_me_task > cmd )
		{
			cmd->m_counter = std::addressof( g_normal_counter );

			if( ++m_tasks_sent < 25'000'000 )
				{
					so_5::send< your_task >( cmd->m_reply_to );
				}
			else
				so_5::send< stop >( cmd->m_reply_to );
		}

	void
	evt_stop_ack( mhood_t< stop_ack > )
		{
			++m_workers_stopped;
			if( m_workers_stopped >= workers_count )
				so_deregister_agent_coop_normally();
		}
};

} /* namespace test */

using namespace test;

int
main()
{
	run_with_time_limit( [] {
			so_5::launch(
				[]( so_5::environment_t & env )
				{
					env.introduce_coop(
						so_5::disp::active_obj::make_dispatcher( env ).binder(),
						[]( so_5::coop_t & coop ) {
							auto * manager = coop.make_agent< manager_t >();

							for( int i = 0; i < workers_count; ++i )
							{
								coop.make_agent< worker_t >( manager->so_direct_mbox() );
							}
						} );
				},
				[]( so_5::environment_params_t & params )
				{
					params.queue_locks_defaults_manager(
							so_5::make_defaults_manager_for_simple_locks() );
				} );
		},
		30000 );

	std::cout
			<< " normal counter = " << g_normal_counter.load() << "\n"
			<< "failure counter = " << g_failure_counter.load() << std::endl;

	ensure_or_die( 0 == g_failure_counter.load(),
			"g_failure_counter has to be 0" );

	return 0;
}

