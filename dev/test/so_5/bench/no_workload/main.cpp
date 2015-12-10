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

#include <various_helpers_1/benchmark_helpers.hpp>
#include <various_helpers_1/cmd_line_args_helpers.hpp>

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
				.event< msg_shutdown >( &a_contoller_t::evt_shutdown );
		}

		void
		so_evt_start()
		{
			m_timer = so_environment().schedule_timer< msg_shutdown >(
					so_direct_mbox(),
					1000,
					1000 );
		}

		void
		evt_shutdown()
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
};

std::size_t 
default_thread_pool_size()
{
	auto c = std::thread::hardware_concurrency();
	if( !c )
		c = 4;

	return c;
}

so_5::dispatcher_unique_ptr_t
create_dispatcher( const cfg_t & cfg )
{
	const auto threads = cfg.m_threads ?
			cfg.m_threads : default_thread_pool_size();

	if( dispatcher_t::adv_thread_pool == cfg.m_dispatcher )
		return so_5::disp::adv_thread_pool::create_disp( threads );

	return so_5::disp::thread_pool::create_disp( threads );
}

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
						new a_contoller_t( env, cfg ) );
			},
			[cfg]( so_5::environment_params_t & params )
			{
				params.add_named_dispatcher(
					"dispatcher",
					create_dispatcher( cfg ) );

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

