/*
 * A test for inability to deregister parent cooperation during
 * child cooperation registration.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include "test/so_5/svc/a_time_sentinel.hpp"

struct msg_parent_started : public so_5::signal_t {};

struct msg_initiate_dereg : public so_5::signal_t {};

struct msg_check_signal : public so_5::signal_t {};

struct msg_shutdown : public so_5::signal_t {};

class a_child_t : public so_5::agent_t
{
	public :
		a_child_t(
			so_5::environment_t & env,
			const so_5::mbox_t & self_mbox,
			const so_5::mbox_t & parent_mbox )
			:	so_5::agent_t( env )
			,	m_mbox( self_mbox )
			,	m_parent_mbox( parent_mbox )
			,	m_so_evt_finish_passed( false )
		{}

		void
		so_define_agent() override
		{
			so_subscribe( m_mbox ).event(
					&a_child_t::evt_check_signal );

			m_parent_mbox->run_one()
					.wait_for( std::chrono::milliseconds( 100 ) )
					.sync_get< msg_initiate_dereg >();

			so_5::send< msg_check_signal >( m_mbox );
		}

		void
		so_evt_finish() override
		{
			m_so_evt_finish_passed = true;
		}

		void
		evt_check_signal(mhood_t< msg_check_signal >)
		{
			if( m_so_evt_finish_passed )
				throw std::runtime_error(
						"evt_check_signal after so_evt_finish" );
		}

	private :
		const so_5::mbox_t m_mbox;
		const so_5::mbox_t m_parent_mbox;

		bool m_so_evt_finish_passed;
};

class a_parent_t : public so_5::agent_t
{
	public :
		a_parent_t(
			so_5::environment_t & env,
			const so_5::mbox_t & mbox )
			:	so_5::agent_t( env )
			,	m_mbox( mbox )
		{}

		void
		so_define_agent() override
		{
			so_subscribe( m_mbox ).event(
					&a_parent_t::evt_initiate_dereg );
		}

		void
		so_evt_start() override
		{
			so_5::send< msg_parent_started >( m_mbox );
		}

		void
		evt_initiate_dereg(mhood_t< msg_initiate_dereg >)
		{
			so_deregister_agent_coop_normally();
		}

	private :
		const so_5::mbox_t m_mbox;
};

class a_driver_t : public so_5::agent_t
{
	public :
		a_driver_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
			,	m_mbox( env.create_mbox() )
		{}

		void
		so_define_agent() override
		{
			so_subscribe( m_mbox ).event(
					&a_driver_t::evt_parent_started );

			so_subscribe( m_mbox ).event(
					&a_driver_t::evt_shutdown );
		}

		void
		so_evt_start() override
		{
			m_parent = so_environment().register_agent_as_coop(
				so_environment().make_agent< a_parent_t >( m_mbox ),
				so_5::disp::active_obj::make_dispatcher( 
						so_environment() ).binder() );
		}

		void
		evt_parent_started(mhood_t< msg_parent_started >)
		{
			auto coop = so_environment().make_coop(
					m_parent,
					so_5::disp::active_obj::make_dispatcher(
							so_environment() ).binder() );

			coop->make_agent< a_child_t >(
					so_environment().create_mbox(),
					m_mbox );

			so_environment().register_coop( std::move( coop ) );

			so_5::send_delayed< msg_shutdown >(
					m_mbox,
					std::chrono::milliseconds(500) );
		}

		void
		evt_shutdown(mhood_t< msg_shutdown >)
		{
			so_environment().stop();
		}

	private :
		const so_5::mbox_t m_mbox;

		so_5::coop_handle_t m_parent;
};

void
init( so_5::environment_t & env )
{
	auto coop = env.make_coop(
			so_5::disp::active_obj::make_dispatcher( env ).binder() );

	coop->make_agent< a_driver_t >();
	coop->make_agent< a_time_sentinel_t >();

	env.register_coop( std::move( coop ) );
}

int
main()
{
	try
	{
		so_5::launch( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

