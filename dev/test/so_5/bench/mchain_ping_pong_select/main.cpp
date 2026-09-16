#include <iostream>
#include <set>
#include <chrono>

#include <cstdio>
#include <cstdlib>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/cmd_line_args_helpers.hpp>
#include <test/3rd_party/various_helpers/benchmark_helpers.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#if defined(__clang__) && (__clang_major__ >= 16)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif

using namespace std::chrono;

namespace benchmark
{

struct cfg_t
{
	unsigned int	m_request_count = 1000;
};

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
							"_test.bench.so_5.mchain_ping_pong_select <options>\n"
							"\noptions:\n"
							"-r, --requests       count of requests to send\n"
							<< std::endl;
					std::exit( 1 );
				}
			else if( is_arg( *current, "-r", "--requests" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_request_count, ++current, last_arg,
						"-r", "count of requests to send" );
		}

	return tmp_cfg;
}

struct msg_ping final : public so_5::signal_t {};
struct msg_pong final : public so_5::signal_t {};

void
pinger_thread_func(
	so_5::mchain_t ping_ch,
	so_5::mchain_t pong_ch,
	unsigned int request_count)
	{
		unsigned int calls = 0;
		for( unsigned int pong_received = 0; pong_received < request_count;
				++pong_received )
			{
				so_5::send< msg_ping >( ping_ch );
				so_5::select(
						so_5::from_all().handle_n( 1 ),
						receive_case( pong_ch,
								[&calls]( so_5::mhood_t< msg_pong > ) {
									++calls;
								} ) );
			}

		ensure_or_die( calls == request_count,
				"mismatch for ponger: calls=" + std::to_string( calls )
				+ ", request_count=" + std::to_string( request_count ) );
	}

void
ponger_thread_func(
	so_5::mchain_t ping_ch,
	so_5::mchain_t pong_ch,
	unsigned int request_count)
	{
		unsigned int calls = 0;
		for( unsigned int ping_received = 0; ping_received < request_count;
				++ping_received )
			{
				so_5::select(
						so_5::from_all().handle_n( 1 ),
						receive_case( ping_ch,
								[&pong_ch, &calls]( so_5::mhood_t< msg_ping > ) {
									++calls;
									so_5::send< msg_pong >( pong_ch );
								} ) );
			}

		ensure_or_die( calls == request_count,
				"mismatch for ponger: calls=" + std::to_string( calls )
				+ ", request_count=" + std::to_string( request_count ) );
	}

void
show_cfg(
	const cfg_t & cfg )
	{
		std::cout << "Configuration: "
			<< "requests: " << cfg.m_request_count
			<< std::endl;
	}

} /* namespace benchmark */

using namespace benchmark;

int
main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = try_parse_cmdline( argc, argv );
		show_cfg( cfg );

		so_5::wrapped_env_t sobj;

		std::thread pinger_thread;
		std::thread ponger_thread;

		auto joiner = so_5::auto_join( pinger_thread, ponger_thread );

		auto ping_ch = create_mchain( sobj.environment() );
		auto pong_ch = create_mchain( sobj.environment() );

		auto ch_closer = so_5::auto_close_drop_content( ping_ch, pong_ch );

		duration_meter_t meter{ "ping-pong on sync select" };

		pinger_thread = std::thread{ &pinger_thread_func,
				ping_ch, pong_ch, cfg.m_request_count };

		ponger_thread = std::thread{ &ponger_thread_func,
				ping_ch, pong_ch, cfg.m_request_count };

		pinger_thread.join();
		ponger_thread.join();

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

