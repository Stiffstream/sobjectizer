/*
 * A test for simple_mtsafe_st_env_infastructure with recreation of
 * 10000 of agents (each of them eats 1MiB memory).
 *
 * If there is a problem with deallocation of agent from deregistered
 * coop then this test will eat a lot of memory.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

using namespace std;

class actor_t : public so_5::agent_t
	{
		struct next_turn final : public so_5::signal_t {};

		std::array< char, 1024u * 1024u > m_dead_data;

		void
		on_next_turn( mhood_t<next_turn> )
			{
				so_5::send< next_turn >( *this );
			}

	public :
		actor_t( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
			{
				so_subscribe_self().event( &actor_t::on_next_turn );
			}

		void
		so_evt_start() override
			{
				// Touch every 1000th byte in dead-data.
				for( std::size_t i = 0u; i < m_dead_data.size(); i += 1000u )
					m_dead_data[ i ] = static_cast<char>(i % 125u);

				so_5::send< next_turn >( *this );
			}
	};

class manager_t final
	{
		enum class state_t {
			wait_first_reg_notify,
			wait_second_reg_notify,
			wait_first_dereg_notify
		};

		state_t m_state{ state_t::wait_first_reg_notify };

		std::size_t m_agents_created{};

		void
		next_turn( so_5::environment_t & env )
			{
				m_state = state_t::wait_first_reg_notify;

				env.introduce_coop( [this]( so_5::coop_t & coop ) {
						coop.add_reg_notificator(
							[this]( so_5::environment_t & environment,
								const so_5::coop_handle_t & handle ) noexcept
							{
								ensure_or_die( m_state == state_t::wait_first_reg_notify,
										"m_state != state_t::wait_first_reg_notify" );

								++m_agents_created;

								environment.deregister_coop( handle,
										so_5::dereg_reason::normal );

								m_state = state_t::wait_second_reg_notify;
							} );

						coop.add_reg_notificator(
							[this]( so_5::environment_t & /*env*/,
								const so_5::coop_handle_t & /*handle*/ ) noexcept
							{
								ensure_or_die( m_state == state_t::wait_second_reg_notify,
										"m_state != state_t::wait_second_reg_notify" );

								m_state = state_t::wait_first_dereg_notify;
							} );

						coop.add_dereg_notificator(
							[this]( so_5::environment_t & environment,
								const so_5::coop_handle_t & /*handle*/,
								const so_5::coop_dereg_reason_t & /*reason*/ ) noexcept
							{
								ensure_or_die( m_state == state_t::wait_first_dereg_notify,
										"m_state != state_t::wait_first_dereg_notify" );

								m_state = state_t::wait_first_reg_notify;

								if( m_agents_created < 10000u )
								{
									std::cout << m_agents_created << "\r" << std::flush;

									next_turn( environment );
								}
								else
									environment.stop();
							} );

						coop.make_agent< actor_t >();
					} );
			}
	
	public :
		manager_t() = default;

		void
		start( so_5::environment_t & env )
			{
				next_turn( env );
			}
	};

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				manager_t manager;
				so_5::launch(
					[&]( so_5::environment_t & env ) {
						manager.start( env );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_not_mtsafe::factory() );
					} );
			},
			240 );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

