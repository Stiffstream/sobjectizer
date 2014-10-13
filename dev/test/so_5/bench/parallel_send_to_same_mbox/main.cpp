/*
 * A benchmark of parallel send from different agents to the same mbox.
 *
 * There is no a receiver for the messages. Benchmark shows only
 * the price of parallel access to the mbox.
 */

#include <iostream>
#include <iterator>
#include <numeric>
#include <cstdlib>

#include <so_5/all.hpp>

#include <various_helpers_1/benchmark_helpers.hpp>

struct msg_send : public so_5::rt::signal_t {};

struct msg_complete : public so_5::rt::signal_t {};

class a_sender_t
	:	public so_5::rt::agent_t
	{
	public :
		a_sender_t(
			so_5::rt::environment_t & env,
			const so_5::rt::mbox_ref_t & mbox,
			unsigned int send_count )
			:	so_5::rt::agent_t( env )
			,	m_mbox( mbox )
			,	m_send_count( send_count )
			{}

		virtual void
		so_evt_start()
			{
				for( unsigned int i = 0; i != m_send_count; ++i )
					m_mbox->deliver_signal< msg_send >();

				m_mbox->deliver_signal< msg_complete >();
			}

	private :
		const so_5::rt::mbox_ref_t m_mbox;

		unsigned int m_send_count;
	};

class a_shutdowner_t
	:	public so_5::rt::agent_t
	{
	public :
		a_shutdowner_t(
			so_5::rt::environment_t & env,
			const so_5::rt::mbox_ref_t & mbox,
			unsigned int sender_count )
			:	so_5::rt::agent_t( env )
			,	m_sender_count( sender_count )
			{
				so_subscribe( mbox ).event< msg_complete >(
					[=] {
						m_sender_count -= 1;
						if( !m_sender_count )
							so_environment().stop();
					} );
			}

	private :
		unsigned int m_sender_count;
	};

void
init(
	so_5::rt::environment_t & env,
	unsigned int agent_count,
	unsigned int send_count )
	{
		auto mbox = env.create_local_mbox();

		auto coop = env.create_coop( "benchmark",
				so_5::disp::active_obj::create_disp_binder( "active_obj" ) );
		
		for( unsigned int i = 0; i != agent_count; ++i )
			coop->add_agent( new a_sender_t( env, mbox, send_count ) );

		coop->add_agent( new a_shutdowner_t( env, mbox, agent_count ),
				so_5::rt::create_default_disp_binder() );

		env.register_coop( std::move( coop ) );
	}

int
main( int argc, char ** argv )
{
	if( 3 != argc )
	{
		std::cout << "Usage:\n\n"
			"parallel_sent_to_same_mbox <agent_count> <send_count>"
			<< std::endl;
		return 1;
	}

	try
	{
		const unsigned int agent_count = std::atoi( argv[1] );
		const unsigned int send_count = std::atoi( argv[2] );

		benchmarker_t benchmark;
		benchmark.start();

		so_5::launch(
			[agent_count, send_count]( so_5::rt::environment_t & env )
			{
				init( env, agent_count, send_count );
			},
			[]( so_5::rt::environment_params_t & params )
			{
				params.add_named_dispatcher( "active_obj",
					so_5::disp::active_obj::create_disp() );
			} );

		benchmark.finish_and_show_stats(
				static_cast< unsigned long long >(agent_count) * send_count,
				"sends" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 2;
	}

	return 0;
}

