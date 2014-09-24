#include <iostream>
#include <set>
#include <chrono>

#include <cstdio>

#include <so_5/api/h/api.hpp>
#include <so_5/rt/h/rt.hpp>

#include <so_5/disp/active_obj/h/pub.hpp>

#include <various_helpers_1/cmd_line_args_helpers.hpp>

using namespace std::chrono;

struct	cfg_t
{
	unsigned int	m_request_count;

	bool	m_active_objects;

	bool	m_direct_mboxes;

	cfg_t()
		:	m_request_count( 1000 )
		,	m_active_objects( false )
		,	m_direct_mboxes( false )
		{}
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
							"-h, --help           show this help"
							<< std::endl;
					std::exit( 1 );
				}
			else if( is_arg( *current, "-a", "--active-objects" ) )
				tmp_cfg.m_active_objects = true;
			else if( is_arg( *current, "-d", "--direct-mboxes" ) )
				tmp_cfg.m_direct_mboxes = true;
			else if( is_arg( *current, "-r", "--requests" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_request_count, ++current, last,
						"-r", "count of requests to send" );
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

struct msg_data : public so_5::rt::signal_t {};

class a_pinger_t
	:	public so_5::rt::agent_t
	{
		typedef so_5::rt::agent_t base_type_t;
	
	public :
		a_pinger_t(
			so_5::rt::environment_t & env,
			const cfg_t & cfg,
			measure_result_t & measure_result )
			:	base_type_t( env )
			,	m_cfg( cfg )
			,	m_measure_result( measure_result )
			,	m_requests_sent( 0 )
			{}

		void
		set_self_mbox( const so_5::rt::mbox_ref_t & mbox )
			{
				m_self_mbox = mbox;
			}

		void
		set_ponger_mbox( const so_5::rt::mbox_ref_t & mbox )
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
			const so_5::rt::event_data_t< msg_data > & cmd )
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
		so_5::rt::mbox_ref_t m_self_mbox;
		so_5::rt::mbox_ref_t m_ponger_mbox;

		const cfg_t m_cfg;
		measure_result_t & m_measure_result;

		unsigned int m_requests_sent;

		void
		send_ping()
			{
				m_ponger_mbox->deliver_signal< msg_data >();
			}
	};

class a_ponger_t
	:	public so_5::rt::agent_t
	{
		typedef so_5::rt::agent_t base_type_t;
	
	public :
		a_ponger_t(
			so_5::rt::environment_t & env )
			:	base_type_t( env )
			{}

		void
		set_self_mbox( const so_5::rt::mbox_ref_t & mbox )
			{
				m_self_mbox = mbox;
			}

		void
		set_pinger_mbox( const so_5::rt::mbox_ref_t & mbox )
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
			const so_5::rt::event_data_t< msg_data > & cmd )
			{
				m_pinger_mbox->deliver_signal< msg_data >();
			}

	private :
		so_5::rt::mbox_ref_t m_self_mbox;
		so_5::rt::mbox_ref_t m_pinger_mbox;
	};

void
show_cfg(
	const cfg_t & cfg )
	{
		std::cout << "Configuration: "
			<< "active objects: " << ( cfg.m_active_objects ? "yes" : "no" )
			<< ", direct mboxes: " << ( cfg.m_direct_mboxes ? "yes" : "no" )
			<< ", requests: " << cfg.m_request_count
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

		const unsigned int total_msg_count = cfg.m_request_count * 2;

		double price = total_msec / total_msg_count / 1000.0;
		double throughtput = 1 / price;

		std::cout.precision( 10 );
		std::cout <<
			"total time: " << total_msec / 1000.0 << 
			", messages sent: " << total_msg_count <<
			", price: " << price <<
			", throughtput: " << throughtput << std::endl;
	}

class test_env_t
	{
	public :
		test_env_t( const cfg_t & cfg )
			:	m_cfg( cfg )
			{}

		void
		init( so_5::rt::environment_t & env )
			{
				auto binder = ( m_cfg.m_active_objects ?
						so_5::disp::active_obj::create_disp_binder( "active_obj" ) :
						so_5::rt::create_default_disp_binder() );

				auto coop = env.create_coop( "test", std::move(binder) );

				auto a_pinger = new a_pinger_t( env, m_cfg, m_result );
				auto a_ponger = new a_ponger_t( env );

				auto pinger_mbox = m_cfg.m_direct_mboxes ?
						a_pinger->so_direct_mbox() : env.create_local_mbox();
				auto ponger_mbox = m_cfg.m_direct_mboxes ?
						a_ponger->so_direct_mbox() : env.create_local_mbox();

				a_pinger->set_self_mbox( pinger_mbox );
				a_pinger->set_ponger_mbox( ponger_mbox );

				a_ponger->set_self_mbox( ponger_mbox );
				a_ponger->set_pinger_mbox( pinger_mbox );

				coop->add_agent( a_pinger );
				coop->add_agent( a_ponger );

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
		show_cfg( cfg );

		test_env_t test_env( cfg );

		so_5::launch(
			[&]( so_5::rt::environment_t & env )
			{
				test_env.init( env );
			},
			[&]( so_5::rt::environment_params_t & params )
			{
				if( cfg.m_active_objects )
					params.add_named_dispatcher(
							"active_obj",
							so_5::disp::active_obj::create_disp() );
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

