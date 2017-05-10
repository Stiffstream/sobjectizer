#include <iostream>
#include <set>
#include <chrono>

#include <cstdio>
#include <cstdlib>

#include <so_5/all.hpp>

#include <various_helpers_1/cmd_line_args_helpers.hpp>
#include <various_helpers_1/benchmark_helpers.hpp>

using namespace std::chrono;

enum class env_type_t
{
	default_mt,
	simple_mtsafe,
	simple_not_mtsafe
};

struct	cfg_t
{
	unsigned int	m_request_count = 1000;

	bool	m_active_objects = false;
	bool	m_simple_lock = false;

	bool	m_direct_mboxes = false;

	bool	m_message_limits = false;

	bool	m_track_activity = false;

	env_type_t m_env = env_type_t::default_mt;
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
							"_test.bench.so_5.ping_pong <options>\n"
							"\noptions:\n"
							"-a, --active-objects agents should be active objects\n"
							"-r, --requests       count of requests to send\n"
							"-d, --direct-mboxes  use direct(mpsc) mboxes for agents\n"
							"-l, --message-limits use message limits for agents\n"
							"-s, --simple-lock    use simple lock factory for event queue\n"
							"-T, --track-activity turn work thread activity tracking on\n"
							"-e, --env            environment infrastructure to be used:\n"
							"                       default_mt (default),\n"
							"                       simple_mtsafe,\n"
							"                       simple_not_mtsafe\n"
							"-h, --help           show this help"
							<< std::endl;
					std::exit( 1 );
				}
			else if( is_arg( *current, "-a", "--active-objects" ) )
				tmp_cfg.m_active_objects = true;
			else if( is_arg( *current, "-d", "--direct-mboxes" ) )
				tmp_cfg.m_direct_mboxes = true;
			else if( is_arg( *current, "-l", "--message-limits" ) )
				tmp_cfg.m_message_limits = true;
			else if( is_arg( *current, "-s", "--simple-lock" ) )
				tmp_cfg.m_simple_lock = true;
			else if( is_arg( *current, "-r", "--requests" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_request_count, ++current, last,
						"-r", "count of requests to send" );
			else if( is_arg( *current, "-T", "--track-activity" ) )
				tmp_cfg.m_simple_lock = true;
			else if( is_arg( *current, "-e", "--env" ) )
				{
					std::string env_type_literal;
					mandatory_arg_to_value(
							env_type_literal,
							++current, last,
							"-e", "type of environment infrastructure" );
					if( "default_mt" == env_type_literal )
						tmp_cfg.m_env = env_type_t::default_mt;
					else if( "simple_mtsafe" == env_type_literal )
						tmp_cfg.m_env = env_type_t::simple_mtsafe;
					else if( "simple_not_mtsafe" == env_type_literal )
					{
						tmp_cfg.m_env = env_type_t::simple_not_mtsafe;
					}
					else
						throw std::runtime_error( "unknown type of "
								"environment infrastructure: " + env_type_literal );
				}
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

struct msg_data : public so_5::signal_t {};

class a_pinger_t
	:	public so_5::agent_t
	{
		typedef so_5::agent_t base_type_t;
	
	public :
		a_pinger_t(
			context_t ctx,
			const cfg_t & cfg,
			measure_result_t & measure_result )
			:	base_type_t( prepare_context( ctx, cfg ) )
			,	m_cfg( cfg )
			,	m_measure_result( measure_result )
			,	m_requests_sent( 0 )
			{}

		void
		set_self_mbox( const so_5::mbox_t & mbox )
			{
				m_self_mbox = mbox;
			}

		void
		set_ponger_mbox( const so_5::mbox_t & mbox )
			{
				m_ponger_mbox = mbox;
			}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_self_mbox ).event( &a_pinger_t::evt_pong );
			}

		virtual void
		so_evt_start()
			{
				m_measure_result.m_start_time = steady_clock::now();

				send_ping();
			}

		void
		evt_pong(
			const so_5::event_data_t< msg_data > & )
			{
				++m_requests_sent;
				if( m_requests_sent < m_cfg.m_request_count )
					send_ping();
				else
					{
						m_measure_result.m_finish_time = steady_clock::now();
						so_environment().stop();
					}
			}

	private :
		so_5::mbox_t m_self_mbox;
		so_5::mbox_t m_ponger_mbox;

		const cfg_t m_cfg;
		measure_result_t & m_measure_result;

		unsigned int m_requests_sent;

		void
		send_ping()
			{
				m_ponger_mbox->deliver_signal< msg_data >();
			}

		static context_t
		prepare_context( context_t ctx, const cfg_t & cfg )
			{
				if( cfg.m_message_limits )
					return ctx + limit_then_abort< msg_data >( 1 );
				else
					return ctx;
			}
	};

class a_ponger_t
	:	public so_5::agent_t
	{
		typedef so_5::agent_t base_type_t;
	
	public :
		a_ponger_t(
			context_t ctx,
			const cfg_t & cfg )
			:	base_type_t( prepare_context( ctx, cfg ) )
			{}

		void
		set_self_mbox( const so_5::mbox_t & mbox )
			{
				m_self_mbox = mbox;
			}

		void
		set_pinger_mbox( const so_5::mbox_t & mbox )
			{
				m_pinger_mbox = mbox;
			}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_self_mbox ).event( &a_ponger_t::evt_ping );
			}

		void
		evt_ping(
			const so_5::event_data_t< msg_data > & )
			{
				m_pinger_mbox->deliver_signal< msg_data >();
			}

	private :
		so_5::mbox_t m_self_mbox;
		so_5::mbox_t m_pinger_mbox;

		static context_t
		prepare_context( context_t ctx, const cfg_t & cfg )
			{
				if( cfg.m_message_limits )
					return ctx + limit_then_abort< msg_data >( 1 );
				else
					return ctx;
			}
	};

void
show_cfg(
	const cfg_t & cfg )
	{
		std::cout << "Configuration: "
			<< "active objects: " << ( cfg.m_active_objects ? "yes" : "no" )
			<< ", direct mboxes: " << ( cfg.m_direct_mboxes ? "yes" : "no" )
			<< ", limits: " << ( cfg.m_message_limits ? "yes" : "no" )
			<< ", locks: " << ( cfg.m_simple_lock ? "simple" : "combined" )
			<< ", requests: " << cfg.m_request_count
			<< ", activity tracking: " << ( cfg.m_track_activity ? "on" : "off" )
			<< ", env: " << ( env_type_t::default_mt == cfg.m_env ?
					"mt" : ( env_type_t::simple_mtsafe == cfg.m_env ?
							"mtsafe" : "not_mtsafe" ) )
			<< std::endl;
	}

void
show_result(
	const cfg_t & cfg,
	const measure_result_t & result )
	{
		auto total_msec =
				duration_cast< milliseconds >( result.m_finish_time -
						result.m_start_time ).count();

		const unsigned int total_msg_count = cfg.m_request_count * 2;

		double price = static_cast< double >( total_msec ) / total_msg_count / 1000.0;
		double throughtput = 1 / price;

		benchmarks_details::precision_settings_t precision{ std::cout, 10 };
		std::cout <<
			"total time: " << total_msec / 1000.0 << 
			", messages sent: " << total_msg_count <<
			", price: " << price <<
			", throughtput: " << throughtput << std::endl;
	}

void
ensure_valid_cfg( const cfg_t & cfg )
	{
		if( cfg.m_active_objects && env_type_t::simple_not_mtsafe == cfg.m_env )
			throw std::runtime_error( "invalid config: active objects can't be"
					" used with simple_not_mtsafe environment infrastructure" );
	}

class test_env_t
	{
	public :
		test_env_t( const cfg_t & cfg )
			:	m_cfg( cfg )
			{}

		void
		init( so_5::environment_t & env )
			{
				auto binder = ( m_cfg.m_active_objects ?
						so_5::disp::active_obj::create_disp_binder( "active_obj" ) :
						so_5::make_default_disp_binder( env ) );

				auto coop = env.create_coop( "test", std::move(binder) );

				auto a_pinger = coop->make_agent< a_pinger_t >( m_cfg, m_result );
				auto a_ponger = coop->make_agent< a_ponger_t >( m_cfg );

				auto pinger_mbox = m_cfg.m_direct_mboxes ?
						a_pinger->so_direct_mbox() : env.create_mbox();
				auto ponger_mbox = m_cfg.m_direct_mboxes ?
						a_ponger->so_direct_mbox() : env.create_mbox();

				a_pinger->set_self_mbox( pinger_mbox );
				a_pinger->set_ponger_mbox( ponger_mbox );

				a_ponger->set_self_mbox( ponger_mbox );
				a_ponger->set_pinger_mbox( pinger_mbox );

				env.register_coop( std::move( coop ) );
			}

		void
		process_results() const
			{
				show_result( m_cfg, m_result );
			}

	private :
		const cfg_t m_cfg;
		measure_result_t m_result;
	};

int
main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = try_parse_cmdline( argc, argv );
		ensure_valid_cfg( cfg );
		show_cfg( cfg );

		test_env_t test_env( cfg );

		so_5::launch(
			[&]( so_5::environment_t & env )
			{
				test_env.init( env );
			},
			[&]( so_5::environment_params_t & params )
			{
				if( env_type_t::simple_mtsafe == cfg.m_env )
				{
					using namespace so_5::env_infrastructures::simple_mtsafe;
					params.infrastructure_factory( factory() );
				}
				else if( env_type_t::simple_not_mtsafe == cfg.m_env )
				{
					using namespace so_5::env_infrastructures::simple_not_mtsafe;
					params.infrastructure_factory( factory() );
				}

				if( cfg.m_track_activity )
					params.turn_work_thread_activity_tracking_on();

				if( cfg.m_simple_lock )
					params.queue_locks_defaults_manager(
							so_5::make_defaults_manager_for_simple_locks() );

				if( cfg.m_active_objects )
				{
					so_5::disp::active_obj::queue_traits::queue_params_t queue_params;

					params.add_named_dispatcher(
							"active_obj",
							so_5::disp::active_obj::create_disp(
									so_5::disp::active_obj::disp_params_t{}.set_queue_params(
											std::move(queue_params) ) ) );
				}
			} );

		test_env.process_results();

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

