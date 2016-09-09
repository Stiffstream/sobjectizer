/*
 * A benchmark for thread_pool dispatcher.
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

enum class lock_type_t
	{
		combined_lock,
		simple_lock
	};

struct cfg_t
	{
		std::size_t m_cooperations = 1024;
		std::size_t m_agents = 512;
		std::size_t m_messages = 100;
		std::size_t m_demands_at_once = 0;
		std::size_t m_threads = 0;
		bool m_individual_fifo = false;
		dispatcher_t m_dispatcher = dispatcher_t::thread_pool;
		std::size_t m_messages_to_send_at_start = 1;
		lock_type_t m_lock_type = lock_type_t::combined_lock;
		bool m_track_activity = false;
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
							"_test.bench.so_5.thread_pool_disp <options>\n"
							"\noptions:\n"
							"-c, --cooperations      count of cooperations\n"
							"-a, --agents            count of agents in cooperation\n"
							"-m, --messages          count of messages for every agent\n"
							"-d, --demands-at-once   count consequently processed demands\n"
							"-S, --messages-at-start count of messages to be sent at start\n"
							"-t, --threads           size of thread pool\n"
							"-i, --individual-fifo   use individual FIFO for agents\n"
							"-P, --adv-thread-pool   use adv_thread_pool dispatcher\n"
							"-s, --simple-lock       use simple_lock_factory for MPMC queue\n"
							"-T, --track-activity    turn work thread activity tracking on\n"
							"-h, --help              show this description\n"
							<< std::endl;
					std::exit(1);
				}
			else if( is_arg( *current, "-c", "--cooperations" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_cooperations, ++current, last,
						"-c", "count of cooperations" );

			else if( is_arg( *current, "-a", "--arents" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_agents, ++current, last,
						"-a", "count of agents in cooperation" );

			else if( is_arg( *current, "-m", "--messages" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_messages, ++current, last,
						"-m", "count of messages for every agent" );

			else if( is_arg( *current, "-d", "--demands-at-once" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_demands_at_once, ++current, last,
						"-d", "count of consequently processed demands" );

			else if( is_arg( *current, "-S", "--messages-at-start" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_messages_to_send_at_start, ++current, last,
						"-S", "count of messages to be sent at start" );

			else if( is_arg( *current, "-t", "--threads" ) )
				mandatory_arg_to_value(
					tmp_cfg.m_threads, ++current, last,
					"-t", "size of thread pool" );

			else if( is_arg( *current, "-i", "--individual-fifo" ) )
				tmp_cfg.m_individual_fifo = true;

			else if( is_arg( *current, "-P", "--adv-thread-pool" ) )
				tmp_cfg.m_dispatcher = dispatcher_t::adv_thread_pool;

			else if( is_arg( *current, "-s", "--simple-lock" ) )
				tmp_cfg.m_lock_type = lock_type_t::simple_lock;

			else if( is_arg( *current, "-T", "--track-activity" ) )
				tmp_cfg.m_track_activity = true;

			else
				throw std::runtime_error(
						std::string( "unknown argument: " ) + *current );
		}

	if( tmp_cfg.m_messages_to_send_at_start >= tmp_cfg.m_messages )
		throw std::runtime_error(
				"invalid number of messages to be sent at start: " +
				std::to_string( tmp_cfg.m_messages_to_send_at_start ) +
				" (total messages to send: " +
				std::to_string( tmp_cfg.m_messages ) + ")" );

	return tmp_cfg;
}

inline std::size_t
total_messages( const cfg_t & cfg )
{
	const auto total_agents = cfg.m_agents * cfg.m_cooperations;
	return total_agents + total_agents + total_agents * cfg.m_messages;
}

struct msg_start : public so_5::signal_t {};
struct msg_shutdown : public so_5::signal_t {};

struct msg_hello : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t(
			so_5::environment_t & env,
			const so_5::mbox_t & controller_mbox,
			std::size_t total_messages_to_send,
			std::size_t messages_at_start )
			:	so_5::agent_t( env )
			,	m_controller_mbox( controller_mbox )
			,	m_messages_to_send( total_messages_to_send )
			,	m_messages_at_start( messages_at_start )
		{
		}

		virtual void
		so_define_agent()
		{
			so_subscribe( m_controller_mbox )
					.event< msg_start >( &a_test_t::evt_start );
				
			so_subscribe_self().event< msg_hello >( &a_test_t::evt_hello );
		}

		void
		evt_start()
		{
			for( m_messages_sent = 0; m_messages_sent != m_messages_at_start;
					++m_messages_sent )
				so_direct_mbox()->deliver_signal< msg_hello >();
		}

		void
		evt_hello()
		{
			++m_messages_received;
			if( m_messages_received >= m_messages_to_send )
				m_controller_mbox->deliver_signal< msg_shutdown >();
			else if( m_messages_sent < m_messages_to_send )
			{
				so_direct_mbox()->deliver_signal< msg_hello >();
				++m_messages_sent;
			}
		}

	private :
		const so_5::mbox_t m_controller_mbox;

		const std::size_t m_messages_to_send;
		const std::size_t m_messages_at_start;

		std::size_t m_messages_sent = 0;
		std::size_t m_messages_received = 0;
};

class a_contoller_t : public so_5::agent_t
{
	public :
		a_contoller_t(
			so_5::environment_t & env,
			cfg_t cfg )
			:	so_5::agent_t( env )
			,	m_cfg( std::move( cfg ) )
			,	m_working_agents( cfg.m_cooperations * cfg.m_agents )
			,	m_self_mbox( env.create_mbox() )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe( m_self_mbox )
				.event< msg_shutdown >( &a_contoller_t::evt_shutdown );
		}

		void
		so_evt_start()
		{
			create_cooperations();

			m_benchmarker.start();
			m_self_mbox->deliver_signal< msg_start >();
		}

		void
		evt_shutdown()
		{
			--m_working_agents;
			if( !m_working_agents )
			{
				m_benchmarker.finish_and_show_stats(
						total_messages( m_cfg ), "messages" );

				m_shutdown_duration.reset( new duration_meter_t( "shutdown" ) );

				so_environment().stop();
			}
		}

	private :
		const cfg_t m_cfg;
		std::size_t m_working_agents;

		const so_5::mbox_t m_self_mbox;

		benchmarker_t m_benchmarker;

		std::unique_ptr< duration_meter_t > m_shutdown_duration;

		void
		create_cooperations()
		{
			duration_meter_t duration( "creating cooperations" );

			for( std::size_t i = 0; i != m_cfg.m_cooperations; ++i )
			{
				std::ostringstream ss;
				ss << "coop_" << i;

				auto c = so_5::create_child_coop(
						*this,
						ss.str(),
						create_binder() );

				for( std::size_t a = 0; a != m_cfg.m_agents; ++a )
				{
					c->add_agent(
							new a_test_t(
									so_environment(),
									m_self_mbox,
									m_cfg.m_messages,
									m_cfg.m_messages_to_send_at_start ) );
				}
				so_environment().register_coop( std::move( c ) );
			}
		}

		so_5::disp_binder_unique_ptr_t
		create_binder() const
		{
			if( dispatcher_t::thread_pool == m_cfg.m_dispatcher )
			{
				using namespace so_5::disp::thread_pool;
				bind_params_t params;
				if( m_cfg.m_individual_fifo )
					params.fifo( fifo_t::individual );
				if( m_cfg.m_demands_at_once )
					params.max_demands_at_once( m_cfg.m_demands_at_once );
				return create_disp_binder( "thread_pool", params );
			}
			else
			{
				using namespace so_5::disp::adv_thread_pool;
				bind_params_t params;
				if( m_cfg.m_individual_fifo )
					params.fifo( fifo_t::individual );
				return create_disp_binder( "thread_pool", params );
			}
		}
};

std::size_t 
default_thread_pool_size()
{
	auto c = std::thread::hardware_concurrency();
	if( !c )
		c = 4;

	return c;
}

void
show_cfg( const cfg_t & cfg )
{
	std::cout << "coops: " << cfg.m_cooperations
			<< ", agents in coop: " << cfg.m_agents
			<< ", msg per agent: " << cfg.m_messages
			<< " (at start: " << cfg.m_messages_to_send_at_start << ")"
			<< ", total msgs: " << total_messages( cfg )
			<< std::endl;

	std::cout << "\n" "dispatcher: "
			<< (dispatcher_t::thread_pool == cfg.m_dispatcher ?
					"thread_pool" : "adv_thread_pool")
			<< std::endl;
	std::cout << "  MPMC queue lock: "
			<< (lock_type_t::combined_lock == cfg.m_lock_type ?
					"combined" : "simple")
			<< std::endl;

	if( dispatcher_t::thread_pool == cfg.m_dispatcher ) 
	{
		std::cout << "\n*** demands_at_once: ";
		if( cfg.m_demands_at_once )
			std::cout << cfg.m_demands_at_once;
		else
			std::cout << "default ("
				<< so_5::disp::thread_pool::bind_params_t().query_max_demands_at_once()
				<< ")";
	}

	std::cout << "\n*** threads in pool: ";
	if( cfg.m_threads )
		std::cout << cfg.m_threads;
	else
		std::cout << "default ("
			<< default_thread_pool_size() << ")";

	std::cout << "\n*** FIFO: ";
	if( cfg.m_individual_fifo )
		std::cout << "individual";
	else
	{
		std::cout << "default (";
		if( so_5::disp::thread_pool::fifo_t::cooperation ==
				so_5::disp::thread_pool::bind_params_t().query_fifo() )
			std::cout << "cooperation";
		else
			std::cout << "individual";
		std::cout << ")";
	}

	std::cout << "\n*** activity tracking: "
			<< (cfg.m_track_activity ? "on" : "off");

	std::cout << std::endl;
}

so_5::dispatcher_unique_ptr_t
create_dispatcher( const cfg_t & cfg )
{
	const auto threads = cfg.m_threads ?
			cfg.m_threads : default_thread_pool_size();

	if( dispatcher_t::adv_thread_pool == cfg.m_dispatcher )
	{
		using namespace so_5::disp::adv_thread_pool;
		disp_params_t params;
		params.thread_count( threads );
		if( lock_type_t::simple_lock == cfg.m_lock_type )
			params.set_queue_params( queue_traits::queue_params_t{}
					.lock_factory( queue_traits::simple_lock_factory() ) );

		return create_disp( params );
	}
	else
	{
		using namespace so_5::disp::thread_pool;
		disp_params_t params;
		params.thread_count( threads );
		if( lock_type_t::simple_lock == cfg.m_lock_type )
			params.set_queue_params( queue_traits::queue_params_t{}
					.lock_factory( queue_traits::simple_lock_factory() ) );

		return create_disp( params );
	}
}

int
main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = try_parse_cmdline( argc, argv );
		show_cfg( cfg );

		so_5::launch(
			[cfg]( so_5::environment_t & env )
			{
				env.register_agent_as_coop( "test",
						new a_contoller_t( env, cfg ) );
			},
			[cfg]( so_5::environment_params_t & params )
			{
				if( cfg.m_track_activity )
					params.turn_work_thread_activity_tracking_on();

				params.add_named_dispatcher(
					"thread_pool",
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

