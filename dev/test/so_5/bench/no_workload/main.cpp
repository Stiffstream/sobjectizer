/*
 * A test for showing overhead of working threads in abcense of
 * any workload.
 */

#include <iostream>
#include <iterator>
#include <numeric>
#include <chrono>
#include <functional>
#include <sstream>
#include <cstdlib>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/benchmark_helpers.hpp>
#include <test/3rd_party/various_helpers/cmd_line_args_helpers.hpp>

std::size_t 
default_thread_pool_size()
{
	auto c = std::thread::hardware_concurrency();
	if( !c )
		c = 4;

	return c;
}

enum class dispatcher_t
	{
		thread_pool,
		adv_thread_pool
	};

struct cfg_t
	{
		std::size_t m_pause = 30;
		std::size_t m_threads = 0;
		dispatcher_t m_dispatcher = dispatcher_t::thread_pool;
	};

cfg_t
try_parse_cmdline(
	int argc,
	char ** argv )
{
	cfg_t tmp_cfg;

	for( char ** current = &argv[ 1 ], **last = argv + argc;
			current != last;
			++current )
		{
			if( is_arg( *current, "-h", "--help" ) )
				{
					std::cout << "usage:\n"
							"_test.bench.so_5.no_workload <options>\n"
							"\noptions:\n"
							"-d, --dispatcher  type of dispatcher\n"
							"-t, --threads     size of thread pool\n"
							"-p, --pause       timeout before exit (in seconds)\n"
							"-h, --help        show this description\n"
							<< std::endl;
					std::exit(1);
				}
			else if( is_arg( *current, "-d", "--dispatcher" ) )
				{
					std::string v;
					mandatory_arg_to_value( v, ++current, last,
							"-d", "type of dispacther "
									"[thread_pool, adv_thread_pool" );
					if( "thread_pool" == v )
						tmp_cfg.m_dispatcher = dispatcher_t::thread_pool;
					else if( "adv_thread_pool" == v )
						tmp_cfg.m_dispatcher = dispatcher_t::adv_thread_pool;
					else
						throw std::runtime_error(
								"unsupported dispacther type: " + v );
				}
			else if( is_arg( *current, "-t", "--threads" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_threads, ++current, last,
						"-t", "size of thread pool" );
			else if( is_arg( *current, "-p", "--pause" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_pause, ++current, last,
						"-p", "timeout before exit (in seconds)" );
			else
				throw std::runtime_error(
						std::string( "unknown argument: " ) + *current );
		}

	return tmp_cfg;
}

struct msg_shutdown : public so_5::signal_t {};

class a_contoller_t : public so_5::agent_t
{
	public :
		a_contoller_t(
			so_5::environment_t & env,
			cfg_t cfg )
			:	so_5::agent_t( env )
			,	m_cfg( std::move( cfg ) )
			,	m_remaining( m_cfg.m_pause )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe( so_direct_mbox() )
				.event( &a_contoller_t::evt_shutdown );
		}

		void
		so_evt_start()
		{
			m_timer = so_5::send_periodic< msg_shutdown >(
					*this,
					std::chrono::seconds(1),
					std::chrono::seconds(1) );

			create_dispatcher();
		}

		void
		evt_shutdown(mhood_t< msg_shutdown >)
		{
			--m_remaining;
			std::cout << "remaining: " << m_remaining << "s  \r" << std::flush;
			if( !m_remaining )
				so_environment().stop();
		}

	private :
		const cfg_t m_cfg;

		std::size_t m_remaining;

		so_5::timer_thread::timer_id_ref_t m_timer;

		so_5::disp_binder_shptr_t m_binder;

		void
		create_dispatcher()
		{
			const auto threads = m_cfg.m_threads ?
					m_cfg.m_threads : default_thread_pool_size();

			if( dispatcher_t::adv_thread_pool == m_cfg.m_dispatcher )
				m_binder = so_5::disp::adv_thread_pool::make_dispatcher(
						so_environment(), threads ).binder();
			else
				m_binder = so_5::disp::thread_pool::make_dispatcher(
						so_environment(), threads ).binder();
		}
};

int
main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = try_parse_cmdline( argc, argv );

		so_5::launch(
			[cfg]( so_5::environment_t & env )
			{
				env.register_agent_as_coop( "test",
						env.make_agent< a_contoller_t >( cfg ) );
			},
			[cfg]( so_5::environment_params_t & params )
			{
				// This timer thread doesn't consume resources without
				// actual delayed/periodic messages.
				params.timer_thread( so_5::timer_list_factory() );
			});
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

