#pragma once

#include <so_5/rt.hpp>

#include <cstdlib>

class a_time_sentinel_t
	:	public so_5::agent_t
	{
	public :
		struct msg_timeout : public so_5::signal_t {};

		using so_5::agent_t::agent_t;

		virtual void
		so_evt_start()
			{
				so_subscribe_self()
						.event( &a_time_sentinel_t::evt_timeout );

				so_5::send_delayed< msg_timeout >(
						*this,
						std::chrono::milliseconds(5000) );
			}

	private :
		void
		evt_timeout( mhood_t< msg_timeout > )
			{
				std::cerr << "TIMEOUT!!!" << std::endl;

				std::abort();
			}
	};

