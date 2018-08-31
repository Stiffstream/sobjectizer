#include "first.hpp"

#include <so_5/all.hpp>

namespace first
{

namespace
{

class demo_t final : public so_5::agent_t
	{
		struct tick final : public so_5::signal_t {};

		so_5::timer_id_t m_timer;
		unsigned m_received{};

		void
		on_tick( mhood_t<tick> )
		{
			++m_received;
			if( 3 == m_received )
				so_deregister_agent_coop_normally();
		}

	public:
		demo_t( context_t ctx ) : so_5::agent_t( std::move(ctx) )
		{
			so_subscribe_self().event( &demo_t::on_tick );
		}

		void
		so_evt_start() override
		{
			m_timer = so_5::send_periodic< tick >( *this,
					std::chrono::milliseconds(50),
					std::chrono::milliseconds(50) );
		}
	};

} /* namespace anonymous */

FIRST_FUNC void make_coop(void * env)
{
	auto & actual_env = *(reinterpret_cast<so_5::environment_t *>(env));
	actual_env.register_agent_as_coop( "first", actual_env.make_agent<demo_t>() );
}

} /* namespace first */

