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
	adv_thread_pool,
	prio_ot_strictly_ordered
};

enum class queue_lock_type_t
{
	combined,
	simple
};

enum class pool_fifo_t
{
	cooperation,
	individual
};

struct	cfg_t
{
	unsigned int	m_ring_size = 50000;
	unsigned int	m_rounds = 1000;

	bool	m_direct_mboxes = false;

	dispatcher_type_t m_dispatcher_type = dispatcher_type_t::one_thread;

	queue_lock_type_t m_queue_lock_type = queue_lock_type_t::combined;

	pool_fifo_t m_fifo = pool_fifo_t::individual;

	std::size_t m_next_thread_wakeup_threshold = 0;
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
							"-D, --dispatcher     type of dispatcher to be used:\n"
							"                     one_thread,\n"
							"                     thread_pool,\n"
							"                     adv_thread_pool,\n"
							"                     prio_ot_strictly_ordered\n"
							"-L, --queue-lock     type of queue lock to be used:\n"
							"                     combined, simple\n"
							"-f, --fifo           type of fifo for dispatcher with "
								"thread pool:\n"
							"                     cooperation, individual (default)\n"
							"-T, --threshold      value of next_thread_wakeup_threshold for\n"
							"                     thread_pool and adv_thread_pool dispatchers\n"
							"                     (defaule value: 0)\n"
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
					else if( "adv_thread_pool" == name )
						tmp_cfg.m_dispatcher_type = dispatcher_type_t::adv_thread_pool;
					else if( "prio_ot_strictly_ordered" == name )
						tmp_cfg.m_dispatcher_type = dispatcher_type_t::prio_ot_strictly_ordered;
					else
						throw std::runtime_error( "unsupported dispatcher type: " + name );
				}
			else if( is_arg( *current, "-L", "--queue-lock" ) )
				{
					std::string name;
					mandatory_arg_to_value(
							name, ++current, last,
							"-L", "queue lock type" );
					if( "combined" == name )
						tmp_cfg.m_queue_lock_type = queue_lock_type_t::combined;
					else if( "simple" == name )
						tmp_cfg.m_queue_lock_type = queue_lock_type_t::simple;
					else
						throw std::runtime_error( "unsupported queue lock type: " + name );
				}
			else if( is_arg( *current, "-f", "--fifo" ) )
				{
					std::string name;
					mandatory_arg_to_value(
							name, ++current, last,
							"-f", "FIFO type" );
					if( "cooperation" == name )
						tmp_cfg.m_fifo = pool_fifo_t::cooperation;
					else if( "individual" == name )
						tmp_cfg.m_fifo = pool_fifo_t::individual;
					else
						throw std::runtime_error( "unsupported FIFO: " + name );
				}
			else if( is_arg( *current, "-T", "--threshold" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_next_thread_wakeup_threshold, ++current, last,
						"-T", "value of next_thread_wakeup_threshold param" );
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

class a_ring_member_t : public so_5::agent_t
	{
	public :
		struct msg_start : public so_5::signal_t {};

		struct msg_your_turn : public so_5::message_t
			{
				unsigned long long m_request_number;

				msg_your_turn( unsigned long long number )
					:	m_request_number( number )
				{}
			};

		a_ring_member_t(
			so_5::environment_t & env,
			const cfg_t & cfg,
			measure_result_t & measure_result )
			:	so_5::agent_t( env )
			,	m_cfg( cfg )
			,	m_measure_result( measure_result )
			{}

		void
		set_self_mbox( const so_5::mbox_t & mbox )
			{
				m_self_mbox = mbox;
			}

		void
		set_next_mbox( const so_5::mbox_t & mbox )
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
		so_5::mbox_t m_self_mbox;
		so_5::mbox_t m_next_mbox;

		const cfg_t m_cfg;
		measure_result_t & m_measure_result;

		unsigned int m_rounds_passed = 0;
	};

const char *
dispatcher_type_name( dispatcher_type_t t )
	{
		if( dispatcher_type_t::one_thread == t )
			return "one_thread";
		else if( dispatcher_type_t::thread_pool == t )
			return "thread_pool";
		else if( dispatcher_type_t::adv_thread_pool == t )
			return "adv_thread_pool";
		else
			return "prio_ot_strictly_ordered";
	}

const char *
queue_lock_type_name( queue_lock_type_t t )
	{
		if( queue_lock_type_t::combined == t )
			return "combined";
		else
			return "simple";
	}

const char *
fifo_name( pool_fifo_t fifo )
	{
		if( pool_fifo_t::cooperation == fifo )
			return "cooperation";
		else
			return "individual";
	}

void
show_cfg(
	const cfg_t & cfg )
	{
		std::cout << "Configuration:"
			<< "\n\t" "ring size: " << cfg.m_ring_size
			<< "\n\t" "rounds: " << cfg.m_rounds
			<< "\n\t" "direct mboxes: " << ( cfg.m_direct_mboxes ? "yes" : "no" )
			<< "\n\t" "disp: " << dispatcher_type_name( cfg.m_dispatcher_type )
			<< "\n\t" "queue_lock: " << queue_lock_type_name( cfg.m_queue_lock_type );

		if( dispatcher_type_t::thread_pool == cfg.m_dispatcher_type ||
				dispatcher_type_t::adv_thread_pool == cfg.m_dispatcher_type )
			std::cout << "\n\t" "fifo: " << fifo_name( cfg.m_fifo )
					<< "\n\t" "threshold: " << cfg.m_next_thread_wakeup_threshold;

		std::cout << std::endl;
	}

void
show_result(
	const cfg_t & cfg,
	const measure_result_t & result )
	{
		auto total_msec =
				duration_cast< milliseconds >( result.m_finish_time -
						result.m_start_time ).count();

		const unsigned long long total_msg_count =
			static_cast< unsigned long long >( cfg.m_ring_size ) * cfg.m_rounds;

		double price = static_cast< double >( total_msec ) / total_msg_count / 1000.0;
		double throughtput = 1 / price;

		benchmarks_details::precision_settings_t precision{ std::cout, 10 };
		std::cout <<
			"total time: " << total_msec / 1000.0 << 
			", messages sent: " << total_msg_count <<
			", price: " << price <<
			", throughtput: " << throughtput << std::endl;
	}

template<
	typename DISP_PARAMS,
	typename COMBINED_FACTORY,
	typename SIMPLE_FACTORY,
	typename QUEUE_PARAMS_TUNER >
DISP_PARAMS
make_disp_params(
	const cfg_t & cfg,
	COMBINED_FACTORY combined_factory,
	SIMPLE_FACTORY simple_factory,
	QUEUE_PARAMS_TUNER queue_params_tuner )
	{
		DISP_PARAMS disp_params;
		using queue_params_t =
			typename std::decay< decltype(disp_params.queue_params()) >::type;

		disp_params.tune_queue_params( [&]( queue_params_t & p ) {
				if( queue_lock_type_t::simple == cfg.m_queue_lock_type )
					p.lock_factory( simple_factory() );
				else
					p.lock_factory( combined_factory() );
				queue_params_tuner( p );
			} );
		return disp_params;
	}

so_5::disp_binder_unique_ptr_t
create_disp_binder(
	so_5::environment_t & env,
	const cfg_t & cfg )
	{
		const auto t = cfg.m_dispatcher_type;
		if( dispatcher_type_t::one_thread == t )
		{
			using namespace so_5::disp::one_thread;
			auto disp_params = make_disp_params< disp_params_t >(
					cfg,
					[]{ return queue_traits::combined_lock_factory(); },
					[]{ return queue_traits::simple_lock_factory(); },
					[]( queue_traits::queue_params_t & ) {} );
			return create_private_disp( env, "disp", disp_params )->binder();
		}
		else if( dispatcher_type_t::thread_pool == t )
		{
			using namespace so_5::disp::thread_pool;
			auto disp_params = make_disp_params< disp_params_t >(
					cfg,
					[]{ return queue_traits::combined_lock_factory(); },
					[]{ return queue_traits::simple_lock_factory(); },
					[&cfg]( queue_traits::queue_params_t & qp ) {
						qp.next_thread_wakeup_threshold(
								cfg.m_next_thread_wakeup_threshold );
					} );
			return create_private_disp( env, "disp", disp_params )->binder(
					[&cfg]( bind_params_t & p ) {
						if( pool_fifo_t::individual == cfg.m_fifo )
							p.fifo( fifo_t::individual );
					} );
		}
		else if( dispatcher_type_t::adv_thread_pool == t )
		{
			using namespace so_5::disp::adv_thread_pool;
			auto disp_params = make_disp_params< disp_params_t >(
					cfg,
					[]{ return queue_traits::combined_lock_factory(); },
					[]{ return queue_traits::simple_lock_factory(); },
					[&cfg]( queue_traits::queue_params_t & qp ) {
						qp.next_thread_wakeup_threshold(
								cfg.m_next_thread_wakeup_threshold );
					} );
			return create_private_disp( env, "disp", disp_params )->binder(
					[cfg]( bind_params_t & p ) {
						if( pool_fifo_t::individual == cfg.m_fifo )
							p.fifo( fifo_t::individual );
					} );
		}
		else
		{
			using namespace so_5::disp::prio_one_thread::strictly_ordered;
			auto disp_params = make_disp_params< disp_params_t >(
					cfg,
					[]{ return queue_traits::combined_lock_factory(); },
					[]{ return queue_traits::simple_lock_factory(); },
					[]( queue_traits::queue_params_t & ) {} );
			return create_private_disp( env, "disp", disp_params )->binder();
		}
	}

void
create_coop(
	const cfg_t & cfg,
	measure_result_t & result,
	so_5::environment_t & env )
	{
		so_5::mbox_t first_agent_mbox;

		env.introduce_coop(
			create_disp_binder( env, cfg ),
			[&]( so_5::coop_t & coop )
			{
				std::vector< a_ring_member_t * > agents;
				agents.reserve( cfg.m_ring_size );

				std::vector< so_5::mbox_t > mboxes;
				mboxes.reserve( cfg.m_ring_size );

				for( unsigned int i = 0; i != cfg.m_ring_size; ++i )
					{
						auto member = coop.make_agent< a_ring_member_t >( cfg, result );
						agents.push_back( member );

						if( cfg.m_direct_mboxes )
							mboxes.push_back( member->so_direct_mbox() );
						else
							mboxes.push_back( env.create_mbox() );
					}

				for( unsigned int i = 0; i != cfg.m_ring_size; ++i )
					{
						agents[ i ]->set_self_mbox( mboxes[ i ] );
						agents[ i ]->set_next_mbox(
								mboxes[ (i + 1) % cfg.m_ring_size ] );
					}

				first_agent_mbox = mboxes[ 0 ]; 
			} );

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
				[&]( so_5::environment_t & env )
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

