/*
 * A test for adv_thread_pool dispatcher: all thread safe handlers
 * must be finished before any thread unsafe handler.
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

#include <various_helpers_1/time_limited_execution.hpp>

#include "../for_each_lock_factory.hpp"

namespace atp_disp = so_5::disp::adv_thread_pool;

const unsigned int thread_count = 4;

struct msg_shutdown : public so_5::signal_t {};

struct msg_safe_signal : public so_5::signal_t {};

struct msg_unsafe_signal : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
		{
			m_workers = 0;
		}

		virtual void
		so_define_agent()
		{
			so_subscribe_self()
				.event< msg_shutdown >( &a_test_t::evt_shutdown )
				.event< msg_safe_signal >(
						&a_test_t::evt_safe_signal,
						so_5::thread_safe )
				.event< msg_unsafe_signal >( &a_test_t::evt_unsafe_signal );
		}

		void
		so_evt_start()
		{
			for( size_t i = 0; i < 100; ++i )
			{
				for( size_t j = 0; j != thread_count; ++j )
					so_direct_mbox()->deliver_signal< msg_safe_signal >();

				so_direct_mbox()->deliver_signal< msg_unsafe_signal >();
			}

			so_direct_mbox()->deliver_signal< msg_shutdown >();
		}

		void
		evt_shutdown()
		{
			so_environment().stop();
		}

		void
		evt_safe_signal()
		{
			++m_workers;

			while( thread_count != m_workers.load( std::memory_order_acquire ) )
				std::this_thread::yield();
		}

		void
		evt_unsafe_signal()
		{
			if( thread_count != m_workers )
				throw std::runtime_error( "m_workers != thread_count" );

			m_workers = 1;

			using hrt = std::chrono::high_resolution_clock;

			auto f = hrt::now() + std::chrono::milliseconds( 5 );

			do
			{
				if( 1 != m_workers )
					std::runtime_error( "m_workers != 1" );

				std::this_thread::sleep_for( std::chrono::microseconds( 100 ) );

			} while( f > hrt::now() );

			m_workers = 0;
		}

	private :
		std::atomic_uint m_workers;
};

void
run_sobjectizer( atp_disp::queue_traits::lock_factory_t factory )
{
	so_5::launch(
		[&]( so_5::environment_t & env )
		{
			env.register_agent_as_coop(
					"test",
					new a_test_t( env ),
					atp_disp::create_disp_binder(
							"thread_pool",
							atp_disp::bind_params_t() ) );
		},
		[&]( so_5::environment_params_t & params )
		{
			using namespace atp_disp;
			params.add_named_dispatcher(
					"thread_pool",
					create_disp( disp_params_t{}
						.thread_count( thread_count )
						.set_queue_params( queue_traits::queue_params_t{}
							.lock_factory( factory ) ) ) );
		} );
}

int
main()
{
	try
	{
		for_each_lock_factory( []( atp_disp::queue_traits::lock_factory_t factory ) {
			run_with_time_limit(
				[&]()
				{
					run_sobjectizer( factory );
				},
				20,
				"unsafe_after_safe test" );
		} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

