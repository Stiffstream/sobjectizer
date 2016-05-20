#pragma once

#include <so_5/rt/h/rt.hpp>

#include <cstdlib>

class a_time_sentinel_t
	:	public so_5::agent_t
	{
	public :
		struct msg_timeout : public so_5::signal_t {};

		a_time_sentinel_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
			,	m_self_mbox( env.create_mbox() )
			{}

		virtual void
		so_evt_start()
			{
				so_subscribe( m_self_mbox )
						.event( &a_time_sentinel_t::evt_timeout );

				so_environment().single_timer< msg_timeout >(
						m_self_mbox, 5000 );
			}

		void
		evt_timeout( const so_5::event_data_t< msg_timeout > & )
			{
				std::cerr << "TIMEOUT!!!" << std::endl;

				std::abort();
			}

	private :
		const so_5::mbox_t m_self_mbox;
	};

