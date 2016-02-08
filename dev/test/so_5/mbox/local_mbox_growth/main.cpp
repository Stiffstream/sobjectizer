/*
 * A test for local_mbox adaptive subscription container.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
	{
		struct ping : public so_5::signal_t {};
		struct pong : public so_5::signal_t {};

		const so_5::state_t st_creating_coops{ this };
		const so_5::state_t st_destroying_coops{ this };

	public :
		a_test_t( context_t ctx )
			:	so_5::agent_t( ctx )
			,	m_ping_mbox( so_environment().create_mbox() )
			{}

		virtual void
		so_define_agent() override
			{
				this >>= st_creating_coops;

				st_creating_coops
					.event( &a_test_t::evt_coop_registered )
					.event< pong >( &a_test_t::evt_pong_when_creating );

				st_destroying_coops
					.event( &a_test_t::evt_coop_deregistered )
					.event< pong >( &a_test_t::evt_pong_when_destroying );
			}

		virtual void
		so_evt_start() override
			{
				create_next_coop();
			}

	private :
		const so_5::mbox_t m_ping_mbox;

		unsigned int m_iterations_passed = { 0 };
		unsigned int m_last_coop_size = { 1 };
		std::size_t m_live_agents = { 0 };
		std::size_t m_pongs_received = { 0 };

		std::vector< std::string > m_live_coops;

		void
		evt_coop_registered(
			const so_5::msg_coop_registered & )
			{
				so_5::send< ping >( m_ping_mbox );
			}

		void
		evt_pong_when_creating()
			{
				++m_pongs_received;
				if( m_pongs_received == m_live_agents )
					{
						m_pongs_received = 0;
						if( m_live_coops.size() < 16 )
							{
								m_last_coop_size *= 2;
								create_next_coop();
							}
						else
							{
								this >>= st_destroying_coops;
								destroy_next_coop();
							}
					}
			}

		void
		evt_coop_deregistered(
			const so_5::msg_coop_deregistered & )
			{
				if( m_live_coops.empty() )
					{
						++m_iterations_passed;
						if( m_iterations_passed >= 5 )
							so_deregister_agent_coop_normally();
						else
							{
								std::cout << "--- NEXT ITERATION ---" << std::endl;
								this >>= st_creating_coops;
								m_last_coop_size = 1;
								create_next_coop();
							}
					}
				else
					so_5::send< ping >( m_ping_mbox );
			}

		void
		evt_pong_when_destroying()
			{
				++m_pongs_received;
				if( m_pongs_received == m_live_agents )
					{
						m_pongs_received = 0;
						destroy_next_coop();
					}
			}

		void
		create_next_coop()
			{
				std::string coop_name = "child_" + std::to_string( m_last_coop_size );
				auto coop = so_5::create_child_coop( *this, coop_name );

				coop->add_reg_notificator(
						so_5::make_coop_reg_notificator( so_direct_mbox() ) );
				coop->add_dereg_notificator(
						so_5::make_coop_dereg_notificator( so_direct_mbox() ) );

				for( unsigned int i = 0; i != m_last_coop_size; ++i )
					coop->define_agent()
						.event< ping >(
							m_ping_mbox, [this]{ so_5::send< pong >( *this ); } );

				so_environment().register_coop( std::move( coop ) );

				m_live_agents += m_last_coop_size;
				m_live_coops.push_back( coop_name );
			}

		void
		destroy_next_coop()
			{
				std::string coop_to_destroy = m_live_coops.back();
				m_live_coops.pop_back();
				m_live_agents -= m_last_coop_size;
				m_last_coop_size /= 2;

				so_environment().deregister_coop(
						coop_to_destroy,
						so_5::dereg_reason::normal );
			}
	};

void
init( so_5::environment_t & env )
	{
		env.register_agent_as_coop( so_5::autoname,
				env.make_agent< a_test_t >() );
	}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			240,
			"local_mbox_growth" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

