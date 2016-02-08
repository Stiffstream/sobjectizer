/*
 * A simple test for adv_thread_pool dispatcher.
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

struct msg_shutdown : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t(
			so_5::environment_t & env,
			const so_5::mbox_t & shutdowner_mbox )
			:	so_5::agent_t( env )
			,	m_shutdowner_mbox( shutdowner_mbox )
		{
		}

		void
		so_evt_start()
		{
			auto w = ++m_workers;
			if( w > 1 )
			{
				std::cerr << "too many workers: " << w << std::endl;
			}

			std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );

			w = --m_workers;
			if( w != 0 )
			{
				std::cout << "expected no more workers, but: " << w << std::endl;
			}

			m_shutdowner_mbox->deliver_signal< msg_shutdown >();
		}

	private :
		static std::atomic_uint m_workers;

		const so_5::mbox_t m_shutdowner_mbox;
};

std::atomic_uint a_test_t::m_workers = ATOMIC_VAR_INIT( 0 );

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
			so_subscribe( so_direct_mbox() )
				.event< msg_shutdown >( [=] {
						--m_working_agents;
						if( !m_working_agents )
							so_environment().stop();
					} );
		}

	private :
		std::size_t m_working_agents;
};

const std::size_t thread_count = 4;

void
run_sobjectizer( atp_disp::queue_traits::lock_factory_t factory )
{
	so_5::launch(
		[&]( so_5::environment_t & env )
		{
			so_5::mbox_t shutdowner_mbox;
			{
				auto c = env.create_coop( "shutdowner" );
				auto a = c->add_agent( new a_shutdowner_t( env, thread_count ) );
				shutdowner_mbox = a->so_direct_mbox();
				env.register_coop( std::move( c ) );
			}

			atp_disp::bind_params_t params;
			auto c = env.create_coop( "test_agents",
					atp_disp::create_disp_binder( "thread_pool", params ) );
			for( std::size_t i = 0; i != thread_count; ++i )
			{
				c->add_agent( new a_test_t( env, shutdowner_mbox ) );
			}

			env.register_coop( std::move( c ) );
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

