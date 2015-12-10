#include <iostream>
#include <set>
#include <chrono>

#include <cstdio>
#include <cstdlib>

#include <so_5/all.hpp>

#include <various_helpers_1/cmd_line_args_helpers.hpp>
#include <various_helpers_1/benchmark_helpers.hpp>

using namespace std::chrono;

enum class dispatcher_type_t
{
	one_thread,
	thread_pool,
};

struct	cfg_t
{
	unsigned int m_coop_count = 1000;
	unsigned int m_coop_size = 10;

	dispatcher_type_t m_dispatcher_type = dispatcher_type_t::one_thread;
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
							"_test.bench.so_5.coop_dereg <options>\n"
							"\noptions:\n"
							"-c, --coop-count     count of coops to be created\n"
							"-a, --coop-size      size of every coop\n"
							"-D, --dispatcher     type of dispatcher to be used:\n"
							"                     one_thread, thread_pool\n"
							"-h, --help           show this help"
							<< std::endl;
					std::exit( 1 );
				}
			else if( is_arg( *current, "-c", "--coop-count" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_coop_count, ++current, last,
						"-c", "count of coops to be created" );
			else if( is_arg( *current, "-a", "--coop-size" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_coop_size, ++current, last,
						"-a", "count of agents in every coop" );
			else if( is_arg( *current, "-D", "--dispatcher" ) )
				{
					std::string name;
					mandatory_arg_to_value(
							name, ++current, last,
							"-D", "dispatcher type" );
					if( "one_thread" == name )
						tmp_cfg.m_dispatcher_type = dispatcher_type_t::one_thread;
					else if( "thread_pool" == name )
						tmp_cfg.m_dispatcher_type = dispatcher_type_t::thread_pool;
					else
						throw std::runtime_error( "unsupported dispatcher type: " + name );
				}
			else
				throw std::runtime_error(
						std::string( "unknown argument: " ) + *current );
		}

	return tmp_cfg;
}

using binder_generator_t = std::function< so_5::disp_binder_unique_ptr_t() >;

class a_benchmarker_t : public so_5::agent_t
	{
		struct ping : public so_5::signal_t {};
		struct pong : public so_5::signal_t {};

	public :
		a_benchmarker_t(
			context_t ctx,
			cfg_t cfg,
			binder_generator_t binder_generator )
			:	so_5::agent_t{ ctx }
			,	m_cfg{ std::move(cfg) }
			,	m_binder_generator{ std::move(binder_generator) }
			,	m_root_coop_name( "root" )
			{
				m_child_mboxes.reserve( cfg.m_coop_count * cfg.m_coop_size );
			}

		virtual void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( &a_benchmarker_t::evt_root_deregistered )
					.event( &a_benchmarker_t::evt_child_registered )
					.event< pong >( &a_benchmarker_t::evt_pong );
			}

		virtual void
		so_evt_start() override
			{
				// Root for children coops must be registered first.
				so_5::introduce_child_coop(
						*this,
						m_root_coop_name,
						m_binder_generator(),
						[this]( so_5::coop_t & coop ) {
							coop.add_dereg_notificator(
									so_5::make_coop_dereg_notificator(
											so_direct_mbox() ) );
							// Empty agent.
							coop.define_agent();
						} );

				// Now all children coops must be registered.
				m_reg_bench.start();
				for( unsigned int c = 0; c != m_cfg.m_coop_count; ++c )
					register_child_coop();
			}

	private :
		const cfg_t m_cfg;
		const binder_generator_t m_binder_generator;

		const std::string m_root_coop_name;

		std::vector< so_5::mbox_t > m_child_mboxes;

		benchmarker_t m_reg_bench;
		benchmarker_t m_ping_pong_bench;
		benchmarker_t m_dereg_bench;

		unsigned int m_child_coop_reg_count = { 0u };
		unsigned int m_pongs_received = { 0u };

		void
		evt_root_deregistered( const so_5::msg_coop_deregistered & )
			{
				m_dereg_bench.finish_and_show_stats(
						m_cfg.m_coop_count + 1,
						"deregistrations" );

				so_environment().stop();
			}

		void
		evt_child_registered( const so_5::msg_coop_registered & )
			{
				++m_child_coop_reg_count;
				if( m_child_coop_reg_count == m_cfg.m_coop_count )
					{
						// Registration finished.
						m_reg_bench.finish_and_show_stats(
								m_cfg.m_coop_count,
								"registrations" );

						m_ping_pong_bench.start();

						benchmarker_t ping_send_bench;
						ping_send_bench.start();
						for( const auto & c : m_child_mboxes )
							so_5::send< ping >( c );
						ping_send_bench.finish_and_show_stats( m_child_mboxes.size(),
								"pings" );
					}
			}

		void
		evt_pong()
			{
				++m_pongs_received;
				if( m_pongs_received == (m_cfg.m_coop_count * m_cfg.m_coop_size) )
					{
						// Ping-pong stage finished.
						m_ping_pong_bench.finish_and_show_stats( m_pongs_received,
								"ping-pongs" );

						// Initiate deregistration of all children.
						m_dereg_bench.start();
						so_environment().deregister_coop(
								m_root_coop_name,
								so_5::dereg_reason::normal );
					}
			}

		void
		register_child_coop()
			{
				so_environment().introduce_coop(
						m_binder_generator(),
						[this]( so_5::coop_t & coop ) {
							coop.set_parent_coop_name( m_root_coop_name );
							coop.add_reg_notificator(
									so_5::make_coop_reg_notificator(
											so_direct_mbox() ) );

							const auto parent = this;
							for( unsigned int a = 0; a != m_cfg.m_coop_size; ++a )
								{
									// Ad-hoc agent will respond to ping signal.
									auto child = coop.define_agent();
									m_child_mboxes.push_back( child.direct_mbox() );

									child.event< ping >( child, [parent] {
												so_5::send< pong >( *parent );
										} );
								}
						} );
			}
	};

const char *
dispatcher_type_name( dispatcher_type_t t )
	{
		if( dispatcher_type_t::one_thread == t )
			return "one_thread";

		return "thread_pool";
	}

binder_generator_t
make_binder_generator(
	so_5::environment_t & env,
	dispatcher_type_t t )
	{
		if( dispatcher_type_t::one_thread == t )
			{
				using namespace so_5::disp::one_thread;
				auto disp = create_private_disp( env );
				return [disp] { return disp->binder(); };
			}
		else
			{
				using namespace so_5::disp::thread_pool;
				auto disp = create_private_disp( env );
				return [disp] { return disp->binder( bind_params_t{} ); };
			}
	}

void
show_cfg(
	const cfg_t & cfg )
	{
		std::cout << "Configuration: "
			<< "coops: " << cfg.m_coop_count
			<< ", agents_per_coop: " << cfg.m_coop_size
			<< ", disp: " << dispatcher_type_name( cfg.m_dispatcher_type )
			<< std::endl;
	}

void
run_sobjectizer( const cfg_t & cfg )
	{
		so_5::launch( [&cfg]( so_5::environment_t & env ) {
				env.introduce_coop( [&cfg]( so_5::coop_t & coop ) {
						coop.make_agent< a_benchmarker_t >(
								cfg,
								make_binder_generator(
										coop.environment(),
										cfg.m_dispatcher_type ) );
					} );
			} );
	}

int
main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = try_parse_cmdline( argc, argv );
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

