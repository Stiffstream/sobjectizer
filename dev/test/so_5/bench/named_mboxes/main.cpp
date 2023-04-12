/*
 * A test for creation of named mboxes.
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

namespace named_mbox_benchmark
{

struct cfg_t
	{
		std::size_t m_count = 30000;
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
								"_test.bench.so_5.named_mboxes <options>\n"
								"\noptions:\n"
								"-c, --count  number of mboxes to be created\n"
								"-h, --help   show this description\n"
								<< std::endl;
						std::exit(1);
					}
				else if( is_arg( *current, "-c", "--count" ) )
					mandatory_arg_to_value(
							tmp_cfg.m_count, ++current, last,
							"-c", "number of mboxes to be created" );
				else
					throw std::runtime_error(
							std::string( "unknown argument: " ) + *current );
			}

		return tmp_cfg;
	}

struct msg_child_started final : public so_5::message_t
	{
		const so_5::mbox_t m_reply_to;

		explicit msg_child_started( so_5::mbox_t reply_to )
			:	m_reply_to{ std::move(reply_to) }
			{}
	};

struct msg_ack final : public so_5::signal_t {};

struct msg_destroy_child final : public so_5::signal_t {};

class a_child_t final : public so_5::agent_t
	{
		const so_5::mbox_t m_parent;
		const so_5::mbox_t m_self_mbox;

		[[nodiscard]]
		static so_5::mbox_t
		make_self_mbox(
			so_5::environment_t & env,
			std::size_t ordinal )
			{
				return env.create_mbox( "child-mbox-with-ordinal-number=" +
						std::to_string( ordinal ) );
			}

	public:
		a_child_t(
			context_t ctx,
			so_5::mbox_t parent,
			std::size_t ordinal )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_parent{ std::move(parent) }
			,	m_self_mbox{ make_self_mbox( so_environment(), ordinal ) }
			{}

		void
		so_define_agent() override
			{
				so_subscribe( m_self_mbox )
					.event( &a_child_t::evt_ack )
					;
			}

		void
		so_evt_start() override
			{
				so_5::send< msg_child_started >( m_parent, m_self_mbox );
			}

	private:
		void
		evt_ack( mhood_t<msg_ack> )
			{
				so_5::send< msg_destroy_child >( m_parent );
			}
	};

class a_contoller_t : public so_5::agent_t
	{
	public :
		a_contoller_t(
			so_5::environment_t & env,
			cfg_t cfg )
			:	so_5::agent_t( env )
			,	m_cfg( std::move( cfg ) )
			,	m_ordinal( 0 )
			{}

		void
		so_define_agent() override
			{
				so_subscribe( so_direct_mbox() )
					.event( &a_contoller_t::evt_child_started )
					.event( &a_contoller_t::evt_destroy_child )
					;
			}

		void
		so_evt_start() override
			{
				try_create_new_child_or_shut_down();
			}

	private :
		const cfg_t m_cfg;

		std::size_t m_ordinal;

		so_5::coop_handle_t m_child_coop;

		void
		evt_child_started( mhood_t<msg_child_started> cmd )
			{
				so_5::send< msg_ack >( cmd->m_reply_to );
			}

		void
		evt_destroy_child( mhood_t<msg_destroy_child> )
			{
				so_environment().deregister_coop(
						std::move(m_child_coop),
						so_5::dereg_reason::normal );

				try_create_new_child_or_shut_down();
			}

		void
		try_create_new_child_or_shut_down()
			{
				if( m_ordinal >= m_cfg.m_count )
					{
						so_deregister_agent_coop_normally();
					}
				else
					{
						auto coop_holder = so_environment().make_coop( so_coop() );
						coop_holder->make_agent< a_child_t >(
								so_direct_mbox(),
								++m_ordinal );
						m_child_coop = so_environment().register_coop(
								std::move(coop_holder) );
					}
			}
};

} /* namespace named_mbox_benchmark */

int
main( int argc, char ** argv )
{
	using namespace named_mbox_benchmark;

	try
	{
		cfg_t cfg = try_parse_cmdline( argc, argv );

		so_5::launch(
			[cfg]( so_5::environment_t & env )
			{
				env.register_agent_as_coop(
						env.make_agent< a_contoller_t >( cfg ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

