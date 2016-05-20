/*
 * A simple benchmark for so_change_state() performance.
 */

#include <iostream>
#include <iterator>
#include <numeric>
#include <chrono>
#include <cstdlib>

#include <so_5/all.hpp>

#include <various_helpers_1/benchmark_helpers.hpp>

struct msg_dummy : public so_5::signal_t {};

class a_test_t
	:	public so_5::agent_t
	{
	public :
		a_test_t(
			so_5::environment_t & env,
			unsigned int iterations )
			:	so_5::agent_t( env )
			,	m_iterations( iterations )
			{
				m_states.push_back( &st_0 );
				m_states.push_back( &st_1 );
				m_states.push_back( &st_2 );
				m_states.push_back( &st_3 );
				m_states.push_back( &st_4 );
				m_states.push_back( &st_5 );
				m_states.push_back( &st_6 );
				m_states.push_back( &st_7 );
				m_states.push_back( &st_8 );
				m_states.push_back( &st_9 );
			}

		virtual void
		so_define_agent()
			{
				so_subscribe( so_direct_mbox() )
						.in( st_0 )
						.in( st_1 )
						.in( st_2 )
						.in( st_3 )
						.in( st_4 )
						.in( st_5 )
						.in( st_6 )
						.in( st_7 )
						.in( st_8 )
						.in( st_9 ).event( &a_test_t::evt_dummy );
			}

		virtual void
		so_evt_start()
			{
				benchmarker_t bench;
				bench.start();

				unsigned long long changes = 0;
				for( unsigned int i = 0; i != m_iterations; ++i )
					{
						for( auto sp : m_states )
							{
								so_change_state( *sp );
								++changes;
							}
					}

				bench.finish_and_show_stats( changes, "changes" );

				so_environment().stop();
			}

		void
		evt_dummy(
			const so_5::event_data_t< msg_dummy > & )
			{
			}

	private :
		const so_5::state_t st_0{ this, "0" };
		const so_5::state_t st_1{ this, "1" };
		const so_5::state_t st_2{ this, "2" };
		const so_5::state_t st_3{ this, "3" };
		const so_5::state_t st_4{ this, "4" };
		const so_5::state_t st_5{ this, "5" };
		const so_5::state_t st_6{ this, "6" };
		const so_5::state_t st_7{ this, "7" };
		const so_5::state_t st_8{ this, "8" };
		const so_5::state_t st_9{ this, "9" };

		unsigned int m_iterations;

		std::vector< const so_5::state_t * > m_states;
	};

int
main( int argc, char ** argv )
{
	try
	{
		const unsigned int tick_count = 2 == static_cast< unsigned int >(
				argc ? std::atoi( argv[1] ) : 1000);

		so_5::launch(
			[tick_count]( so_5::environment_t & env )
			{
				env.register_agent_as_coop(
					"test",
					new a_test_t( env, tick_count ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

