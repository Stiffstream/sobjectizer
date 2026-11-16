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

		auto ping_ch = create_mchain( sobj.environment() );
		auto pong_ch = create_mchain( sobj.environment() );

		auto ch_closer = so_5::auto_close_drop_content( ping_ch, pong_ch );

		duration_meter_t meter{ "ping-pong on sync select" };

		unsigned int pings_received = 0;
		unsigned int pongs_received = 0;

		so_5::send< msg_ping >( ping_ch );
		for( unsigned int actions = 0; actions != cfg.m_request_count; ++actions )
		{
			so_5::select( so_5::from_all().handle_n( 1 ),
					receive_case( ping_ch,
							[&pings_received, &pong_ch]( so_5::mhood_t< msg_ping > )
							{
								++pings_received;
								so_5::send< msg_pong >( pong_ch );
							} ) );
			so_5::select( so_5::from_all().handle_n( 1 ),
					receive_case( pong_ch,
							[&pongs_received, &ping_ch]( so_5::mhood_t< msg_pong > )
							{
								++pongs_received;
								so_5::send< msg_ping >( ping_ch );
							} ) );
		}

		ensure_or_die( pings_received == cfg.m_request_count,
				"mismatch for pinger: calls=" + std::to_string( pings_received )
				+ ", request_count=" + std::to_string( cfg.m_request_count ) );

		ensure_or_die( pongs_received == cfg.m_request_count,
				"mismatch for ponger: calls=" + std::to_string( pongs_received )
				+ ", request_count=" + std::to_string( cfg.m_request_count ) );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

