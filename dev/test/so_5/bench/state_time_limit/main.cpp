#include <iostream>
#include <set>
#include <chrono>

#include <cstdio>
#include <cstdlib>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/cmd_line_args_helpers.hpp>
#include <test/3rd_party/various_helpers/benchmark_helpers.hpp>

struct cfg_t
{
	unsigned int m_iterations = 50000u;

	int m_time_limit_ms = 100;
};

[[nodiscard]]
cfg_t
try_parse_cmdline(
	int argc,
	char ** argv )
{
	cfg_t tmp_cfg;

	for( char ** current = &argv[ 1 ], **last_arg = argv + argc;
			current != last_arg;
			++current )
		{
			if( is_arg( *current, "-h", "--help" ) )
				{
					std::cout << "usage:\n"
							"_test.bench.so_5.state_time_limit <options>\n"
							"\noptions:\n"
							"-i, --iterations   how many times time_limit has to be called\n"
							"-t, --time-limit   value of time_limit to be used (in milliseconds)\n"
							"-h, --help         show this help"
							<< std::endl;
					std::exit( 1 );
				}
			else if( is_arg( *current, "-i", "--iterations" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_iterations, ++current, last_arg,
						"-i", "number of iterations" );
			else if( is_arg( *current, "-t", "--time-limit" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_time_limit_ms, ++current, last_arg,
						"-t", "number of milliseconds for time_limit" );
			else
				throw std::runtime_error(
						std::string( "unknown argument: " ) + *current );
		}

	return tmp_cfg;
}

class a_benchmarker_t : public so_5::agent_t
	{
		state_t st_working{ this, "working" };

	public :
		a_benchmarker_t(
			context_t ctx,
			cfg_t cfg )
			:	so_5::agent_t{ ctx }
			,	m_cfg{ std::move(cfg) }
			{}

		void
		so_evt_start() override
			{
				this >>= st_working;

				benchmarker_t bench;
				bench.start();

				for( unsigned int i = 0u; i != m_cfg.m_iterations; ++i)
				{
					st_working.time_limit(
							std::chrono::milliseconds{ m_cfg.m_time_limit_ms },
							so_default_state() );
				}

				bench.finish_and_show_stats(
						m_cfg.m_iterations,
						"st_working.time_limit" );

				so_deregister_agent_coop_normally();
			}

	private :
		const cfg_t m_cfg;
	};

void
show_cfg(
	const cfg_t & cfg )
	{
		std::cout << "Configuration: "
				<< "iterations: " << cfg.m_iterations
				<< ", time_limit: " << cfg.m_time_limit_ms << "ms"
				<< std::endl;
	}

void
run_sobjectizer( const cfg_t & cfg )
	{
		so_5::launch( [&cfg]( so_5::environment_t & env ) {
				env.introduce_coop( [&cfg]( so_5::coop_t & coop ) {
						coop.make_agent< a_benchmarker_t >( cfg );
					} );
			},
			[]( so_5::environment_params_t & params )
			{
				// This timer thread doesn't consume resources without
				// actual delayed/periodic messages.
				params.timer_thread( so_5::timer_list_factory() );
			} );
	}

int
main( int argc, char ** argv )
{
	try
	{
		const cfg_t cfg = try_parse_cmdline( argc, argv );
		show_cfg( cfg );

		run_sobjectizer( cfg );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

