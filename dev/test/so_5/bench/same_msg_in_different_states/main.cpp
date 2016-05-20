/*
 */

#include <iostream>
#include <iterator>
#include <numeric>
#include <chrono>
#include <cstdlib>

#include <so_5/all.hpp>

#include <various_helpers_1/benchmark_helpers.hpp>
#include <various_helpers_1/ensure.hpp>

struct msg_tick : public so_5::signal_t {};

class a_test_t
	:	public so_5::agent_t
	{
	public :
		a_test_t(
			so_5::environment_t & env,
			std::size_t states_count,
			int tick_count )
			:	so_5::agent_t( env )
			,	m_self_mbox( env.create_mbox() )
			,	m_tick_count( tick_count )
			,	m_messages_received( 0 )
			{
				for( size_t i = 0; i != states_count; ++i )
					m_states.emplace_back(
							std::make_shared< so_5::state_t >(
									self_ptr(), "noname" ) );

				m_it_current_state = m_states.begin();
			}

		virtual void
		so_define_agent()
			{
				for( auto s : m_states )
					so_subscribe( m_self_mbox )
							.in( *s )
							.event( &a_test_t::evt_tick );
			}

		virtual void
		so_evt_start()
			{
				m_benchmarker.start();

				so_change_state( *(m_states.front()) );

				m_self_mbox->deliver_signal< msg_tick >();
			}

		void
		evt_tick(
			const so_5::event_data_t< msg_tick > & )
			{
				++m_messages_received;
				++m_it_current_state;
				if( m_it_current_state == m_states.end() )
				{
					--m_tick_count;
					m_it_current_state = m_states.begin();
				}

				if( m_tick_count > 0 )
				{
					so_change_state( **m_it_current_state );
					m_self_mbox->deliver_signal< msg_tick >();
				}
				else
				{
					m_benchmarker.finish_and_show_stats(
							m_messages_received,
							"messages" );

					so_environment().stop();
				}
			}

	private :
		const so_5::mbox_t m_self_mbox;

		int m_tick_count;
		std::uint_fast64_t m_messages_received;

		std::vector< std::shared_ptr< so_5::state_t > > m_states;
		std::vector< std::shared_ptr< so_5::state_t > >::iterator m_it_current_state;

		benchmarker_t m_benchmarker;
	};

int
main( int argc, char ** argv )
{
	try
	{
		std::size_t max_states = 16;
		int tick_count = 100000;

		if( 3 == argc )
		{
			max_states = static_cast< std::size_t >(std::atoi( argv[1] ));
			ensure( max_states > 0, "max_states must be >= 1" );

			tick_count = std::atoi( argv[2] );
			ensure( tick_count > 0, "tick_count must be >= 1" );
		}

		for( std::size_t states = 1; states <= max_states; states *= 2 )
		{
			std::cout << "*** benchmark for " << states << " state(s) ***"
				<< std::endl;

			so_5::launch(
				[states, tick_count]( so_5::environment_t & env )
				{
					env.register_agent_as_coop( "test",
							new a_test_t( env, states, tick_count ) );
				} );

			tick_count /= 2;
			if( tick_count < 10 )
				tick_count = 10;
		}
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

