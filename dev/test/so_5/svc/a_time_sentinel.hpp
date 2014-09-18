#if !defined( TEST__SO_5__SVC__TIME_SENTINEL_HPP )
#define TEST__SO_5__SVC__TIME_SENTINEL_HPP

#include <so_5/rt/h/rt.hpp>

#include <cstdlib>

class a_time_sentinel_t
	:	public so_5::rt::agent_t
	{
	public :
		struct msg_timeout : public so_5::rt::signal_t {};

		a_time_sentinel_t(
			so_5::rt::so_environment_t & env )
			:	so_5::rt::agent_t( env )
			,	m_self_mbox( env.create_local_mbox() )
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
		evt_timeout( const so_5::rt::event_data_t< msg_timeout > & evt )
			{
				std::cerr << "TIMEOUT!!!" << std::endl;

				std::abort();
			}

	private :
		const so_5::rt::mbox_ref_t m_self_mbox;
	};

#endif

