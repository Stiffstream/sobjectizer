/*
 * A test for adding a dispatcher to the running SO Environment.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <sstream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_child_info : public so_5::message_t
{
	so_5::current_thread_id_t m_thread_id;

	msg_child_info( so_5::current_thread_id_t id )
		:	m_thread_id( std::move( id ) )
		{}
};

class a_child_t : public so_5::agent_t
{
	public:
		a_child_t(
			so_5::environment_t & env,
			const so_5::mbox_t & parent_mbox )
			:	so_5::agent_t( env )
			,	m_parent_mbox( parent_mbox )
		{}

		virtual void
		so_evt_start()
		{
			m_parent_mbox->deliver_message(
					new msg_child_info( so_5::query_current_thread_id() ) );

			so_deregister_agent_coop_normally();
		}

	private :
		const so_5::mbox_t m_parent_mbox;
};

class a_parent_t : public so_5::agent_t
{
	public :
		a_parent_t(
			so_5::environment_t & env,
			const std::string & dispatcher_name )
			:	so_5::agent_t( env )
			,	m_dispatcher_name( dispatcher_name )
		{}

		virtual void
		so_define_agent() override
		{
			so_subscribe( so_direct_mbox() ).event( &a_parent_t::evt_child_info );
		}

		virtual void
		so_evt_start() override
		{
			m_thread_id = so_5::query_current_thread_id();

			so_environment().add_dispatcher_if_not_exists(
					m_dispatcher_name,
					[]() { return so_5::disp::one_thread::create_disp(); } );

			so_environment().register_agent_as_coop(
					so_coop_name() + "::child",
					new a_child_t( so_environment(), so_direct_mbox() ),
					so_5::disp::one_thread::create_disp_binder(
							m_dispatcher_name ) );
		}

		void
		evt_child_info( const msg_child_info & evt )
		{
			if( m_thread_id != evt.m_thread_id )
			{
				std::cerr << so_coop_name() << ": thread_id mismatch! "
						"expected: " << m_thread_id
						<< ", actual: " << evt.m_thread_id << std::endl;
				std::abort();
			}

			so_deregister_agent_coop_normally();
		}

	private :
		const std::string m_dispatcher_name;

		so_5::current_thread_id_t m_thread_id;
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch(
					[]( so_5::environment_t & env )
					{
						for( int i = 0; i < 32; ++i )
						{
							std::ostringstream ss;
							ss << "coop_" << i;

							const std::string name = ss.str();

							env.add_dispatcher_if_not_exists(
								name,
								[]() { return so_5::disp::one_thread::create_disp(); } );

							env.register_agent_as_coop(
								name,
								new a_parent_t( env, name ),
								so_5::disp::one_thread::create_disp_binder( name ) );
						}
					} );
			},
			20,
			"Adding dispatcher on running SO Environment test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

