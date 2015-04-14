#include <iostream>
#include <set>
#include <chrono>

#include <cstdio>
#include <cstdlib>

#include <so_5/all.hpp>

#include <various_helpers_1/cmd_line_args_helpers.hpp>
#include <various_helpers_1/benchmark_helpers.hpp>

using namespace std::chrono;

struct	cfg_t
{
	unsigned int	m_ring_size = 50000;
	unsigned int	m_rounds = 1000;

	bool	m_direct_mboxes = false;
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
							"_test.bench.so_5.agent_ring <options>\n"
							"\noptions:\n"
							"-s, --ring-size      size of agent's ring\n"
							"-r, --rounds         count of full rounds around the ring\n"
							"-d, --direct-mboxes  use direct(mpsc) mboxes for agents\n"
							"-h, --help           show this help"
							<< std::endl;
					std::exit( 1 );
				}
			else if( is_arg( *current, "-d", "--direct-mboxes" ) )
				tmp_cfg.m_direct_mboxes = true;
			else if( is_arg( *current, "-s", "--ring-size" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_ring_size, ++current, last,
						"-s", "size of agent's ring" );
			else if( is_arg( *current, "-r", "--rounds" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_rounds, ++current, last,
						"-r", "count of full rounds around the ring" );
			else
				throw std::runtime_error(
						std::string( "unknown argument: " ) + *current );
		}

	return tmp_cfg;
}

struct	measure_result_t
{
	steady_clock::time_point 	m_start_time;
	steady_clock::time_point	m_finish_time;
};

class a_ring_member_t : public so_5::rt::agent_t
	{
	public :
		struct msg_start : public so_5::rt::signal_t {};

		struct msg_your_turn : public so_5::rt::message_t
			{
				unsigned long long m_request_number;

				msg_your_turn( unsigned long long number )
					:	m_request_number( number )
				{}
			};

		a_ring_member_t(
			so_5::rt::environment_t & env,
			const cfg_t & cfg,
			measure_result_t & measure_result )
			:	so_5::rt::agent_t( env )
			,	m_cfg( cfg )
			,	m_measure_result( measure_result )
			{}

		void
		set_self_mbox( const so_5::rt::mbox_t & mbox )
			{
				m_self_mbox = mbox;
			}

		void
		set_next_mbox( const so_5::rt::mbox_t & mbox )
			{
				m_next_mbox = mbox;
			}

		virtual void
		so_define_agent()
			{
				so_default_state()
					.event< msg_start >( m_self_mbox, &a_ring_member_t::evt_start )
					.event( m_self_mbox, &a_ring_member_t::evt_your_turn );
			}

		void
		evt_start()
			{
				m_measure_result.m_start_time = steady_clock::now();

				so_5::send< msg_your_turn >( m_next_mbox, 0ull );
			}

		void
		evt_your_turn( const msg_your_turn & evt )
			{
				++m_rounds_passed;
				if( m_rounds_passed < m_cfg.m_rounds )
					so_5::send< msg_your_turn >(
							m_next_mbox,
							evt.m_request_number + 1 );
				else
					{
						m_measure_result.m_finish_time = steady_clock::now();
						so_environment().stop();
					}
			}

	private :
		so_5::rt::mbox_t m_self_mbox;
		so_5::rt::mbox_t m_next_mbox;

		const cfg_t m_cfg;
		measure_result_t & m_measure_result;

		unsigned int m_rounds_passed = 0;
	};

void
show_cfg(
	const cfg_t & cfg )
	{
		std::cout << "Configuration: "
			<< "ring size: " << cfg.m_ring_size
			<< ", rounds: " << cfg.m_rounds
			<< ", direct mboxes: " << ( cfg.m_direct_mboxes ? "yes" : "no" )
			<< std::endl;
	}

void
show_result(
	const cfg_t & cfg,
	const measure_result_t & result )
	{
		double total_msec =
				duration_cast< milliseconds >( result.m_finish_time -
						result.m_start_time ).count();

		const unsigned long long total_msg_count =
			static_cast< unsigned long long >( cfg.m_ring_size ) * cfg.m_rounds;

		double price = total_msec / total_msg_count / 1000.0;
		double throughtput = 1 / price;

		benchmarks_details::precision_settings_t precision{ std::cout, 10 };
		std::cout <<
			"total time: " << total_msec / 1000.0 << 
			", messages sent: " << total_msg_count <<
			", price: " << price <<
			", throughtput: " << throughtput << std::endl;
	}

void
create_coop(
	const cfg_t & cfg,
	measure_result_t & result,
	so_5::rt::environment_t & env )
	{
		so_5::rt::mbox_t first_agent_mbox;
		auto coop = env.create_coop( so_5::autoname );

		{
			std::vector< a_ring_member_t * > agents;
			agents.reserve( cfg.m_ring_size );

			std::vector< so_5::rt::mbox_t > mboxes;
			mboxes.reserve( cfg.m_ring_size );

			for( unsigned int i = 0; i != cfg.m_ring_size; ++i )
				{
					auto member = new a_ring_member_t( env, cfg, result );
					coop->add_agent( member );
					agents.push_back( member );

					if( cfg.m_direct_mboxes )
						mboxes.push_back( member->so_direct_mbox() );
					else
						mboxes.push_back( env.create_local_mbox() );
				}

			for( unsigned int i = 0; i != cfg.m_ring_size; ++i )
				{
					agents[ i ]->set_self_mbox( mboxes[ i ] );
					agents[ i ]->set_next_mbox(
							mboxes[ (i + 1) % cfg.m_ring_size ] );
				}

			first_agent_mbox = mboxes[ 0 ]; 
		}

		env.register_coop( std::move( coop ) );

		so_5::send< a_ring_member_t::msg_start >( first_agent_mbox );
	}

int
main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = try_parse_cmdline( argc, argv );
		show_cfg( cfg );

		measure_result_t result;

		so_5::launch(
				[&]( so_5::rt::environment_t & env )
				{
					create_coop( cfg, result, env );
				} );

		show_result( cfg, result );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

