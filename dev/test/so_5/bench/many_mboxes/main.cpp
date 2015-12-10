/*
 * A benchmark for sending messages to M mboxes and to N agents.
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

enum class subscr_storage_type_t
	{
		vector_based,
		map_based,
		hash_table_based
	};

const char *
subscr_storage_name( subscr_storage_type_t type )
	{
		if( subscr_storage_type_t::vector_based == type )
			return "vector_based";
		else if( subscr_storage_type_t::map_based == type )
			return "map_based";
		else
			return "hash_table_based";
	}

struct cfg_t
	{
		std::size_t m_mboxes = 1024;
		std::size_t m_agents = 512;
		std::size_t m_msg_types = 4;
		std::size_t m_iterations = 10;

		subscr_storage_type_t m_subscr_storage =
				subscr_storage_type_t::map_based;

		std::size_t m_vector_subscr_storage_capacity = 8;
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
							"_test.bench.so_5.many_mboxes <options>\n"
							"\noptions:\n"
							"-m, --mboxes           count of mboxes\n"
							"-a, --agents           count of agents\n"
							"-t, --types            count of message types\n"
							"-i, --iterations       count of iterations for every "
									"message type\n"
							"-s, --storage-type     type of subscription storage\n"
							"                       allowed values: vector, map, hash\n"
							"-V, --vector-capacity  initial capacity of vector-based"
									"subscription storage\n"
							"-h, --help        show this description\n"
							<< std::endl;
					std::exit(1);
				}
			else if( is_arg( *current, "-m", "--mboxes" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_mboxes, ++current, last,
						"-m", "count of mboxes" );
			else if( is_arg( *current, "-a", "--agents" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_agents, ++current, last,
						"-a", "count of agents" );
			else if( is_arg( *current, "-t", "--types" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_msg_types, ++current, last,
						"-t", "count of message types" );
			else if( is_arg( *current, "-i", "--iterations" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_iterations, ++current, last,
						"-i", "count of iterations for every message type" );
			else if( is_arg( *current, "-V", "--vector-capacity" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_vector_subscr_storage_capacity, ++current, last,
						"-V", "initial capacity on vector-based"
								"subscription storage" );
			else if( is_arg( *current, "-s", "--storage-type" ) )
				{
					std::string type;
					mandatory_arg_to_value( type, ++current, last,
							"-s", "type of subscription storage" );
					if( "vector" == type )
						tmp_cfg.m_subscr_storage = subscr_storage_type_t::vector_based;
					else if( "map" == type )
						tmp_cfg.m_subscr_storage = subscr_storage_type_t::map_based;
					else if( "hash" == type )
						tmp_cfg.m_subscr_storage = subscr_storage_type_t::hash_table_based;
					else
						throw std::runtime_error(
								std::string( "unsupported subscription storage type: " ) +
										type );
				}
			else
				throw std::runtime_error(
						std::string( "unknown argument: " ) + *current );
		}

	return tmp_cfg;
}

#define DECLARE_SIGNAL_TYPE(I) \
	struct msg_signal_##I : public so_5::signal_t {}

DECLARE_SIGNAL_TYPE(0);
DECLARE_SIGNAL_TYPE(1);
DECLARE_SIGNAL_TYPE(2);
DECLARE_SIGNAL_TYPE(3);
DECLARE_SIGNAL_TYPE(4);
DECLARE_SIGNAL_TYPE(5);
DECLARE_SIGNAL_TYPE(6);
DECLARE_SIGNAL_TYPE(7);
DECLARE_SIGNAL_TYPE(8);
DECLARE_SIGNAL_TYPE(9);
DECLARE_SIGNAL_TYPE(10);
DECLARE_SIGNAL_TYPE(11);
DECLARE_SIGNAL_TYPE(12);
DECLARE_SIGNAL_TYPE(13);
DECLARE_SIGNAL_TYPE(14);
DECLARE_SIGNAL_TYPE(15);
DECLARE_SIGNAL_TYPE(16);
DECLARE_SIGNAL_TYPE(17);
DECLARE_SIGNAL_TYPE(18);
DECLARE_SIGNAL_TYPE(19);
DECLARE_SIGNAL_TYPE(20);
DECLARE_SIGNAL_TYPE(21);
DECLARE_SIGNAL_TYPE(22);
DECLARE_SIGNAL_TYPE(23);
DECLARE_SIGNAL_TYPE(24);
DECLARE_SIGNAL_TYPE(25);
DECLARE_SIGNAL_TYPE(26);
DECLARE_SIGNAL_TYPE(27);
DECLARE_SIGNAL_TYPE(28);
DECLARE_SIGNAL_TYPE(29);
DECLARE_SIGNAL_TYPE(30);
DECLARE_SIGNAL_TYPE(31);

#undef DECLARE_SIGNAL_TYPE

struct msg_start : public so_5::signal_t {};
struct msg_shutdown : public so_5::signal_t {};
struct msg_next_iteration : public so_5::signal_t {};

class a_worker_t
	:	public so_5::agent_t
	{
	public :
		a_worker_t(
			so_5::environment_t & env,
			so_5::subscription_storage_factory_t subscr_storage_factory )
			:	so_5::agent_t( env + subscr_storage_factory )
			,	m_signals_received( 0 )
			{
			}

		virtual void
		evt_signal()
			{
				++m_signals_received;
			}

	private :
		unsigned long m_signals_received;
	};

template< class SIGNAL >
class a_sender_t
	:	public so_5::agent_t
	{
	public :
		a_sender_t(
			so_5::environment_t & env,
			so_5::subscription_storage_factory_t subscr_storage_factory,
			const so_5::mbox_t & common_mbox,
			std::size_t iterations,
			const std::vector< so_5::mbox_t > & mboxes,
			const std::vector< a_worker_t * > & workers )
			:	so_5::agent_t( env + subscr_storage_factory )
			,	m_common_mbox( common_mbox )
			,	m_iterations_left( iterations )
			,	m_mboxes( mboxes )
			{
				for( auto a : workers )
					for( auto & m : m_mboxes )
						a->so_subscribe( m ).template event< SIGNAL >(
								&a_worker_t::evt_signal );
			}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_common_mbox ).template event< msg_start >(
						&a_sender_t::evt_start );

				so_subscribe_self().template event< msg_next_iteration >(
						&a_sender_t::evt_next_iteration );
			}

		void
		evt_start()
			{
				try_start_next_iteration();
			}

		void
		evt_next_iteration()
			{
				try_start_next_iteration();
			}

	private :
		const so_5::mbox_t m_common_mbox;
		std::size_t m_iterations_left;
		const std::vector< so_5::mbox_t > & m_mboxes;

		void
		try_start_next_iteration()
			{
				if( m_iterations_left )
					{
						for( auto & m : m_mboxes )
							so_5::send< SIGNAL >( m );

						initiate_next_iteration();

						--m_iterations_left;
					}
				else
					m_common_mbox->deliver_signal< msg_shutdown >();
			}

		// This method with this strange implementation
		// is necessary because of strange compilation
		// error under GCC 4.9.0.
		void
		initiate_next_iteration()
			{
				so_5::abstract_message_box_t & m = *(so_direct_mbox());
				m.deliver_signal< msg_next_iteration >();
			}
	};

class a_starter_stopper_t
	:	public so_5::agent_t
	{
	public :
		a_starter_stopper_t(
			so_5::environment_t & env,
			so_5::subscription_storage_factory_t subscr_storage_factory,
			const cfg_t & cfg )
			:	so_5::agent_t( env + subscr_storage_factory )
			,	m_subscr_storage_factory( subscr_storage_factory )
			,	m_common_mbox( env.create_mbox() )
			,	m_cfg( cfg )
			,	m_agents_finished( 0 )
			{
				create_sender_factories();
			}

		static const std::size_t max_msg_types = 32;

		virtual void
		so_define_agent()
			{
				so_subscribe( m_common_mbox )
						.event< msg_shutdown >( &a_starter_stopper_t::evt_shutdown );
			}

		virtual void
		so_evt_start()
			{
				std::cout << "* mboxes: " << m_cfg.m_mboxes << "\n"
						<< "* agents: " << m_cfg.m_agents << "\n"
						<< "* msg_types: " << m_cfg.m_msg_types << "\n"
						<< "* iterations: " << m_cfg.m_iterations << "\n"
						<< "* subscr_storage: "
						<< subscr_storage_name( m_cfg.m_subscr_storage )
						<< std::endl;
				if( subscr_storage_type_t::vector_based == m_cfg.m_subscr_storage )
					std::cout << "* vector_initial_capacity: "
							<< m_cfg.m_vector_subscr_storage_capacity
							<< std::endl;

				create_child_coop();

				m_benchmark.start();

				m_common_mbox->deliver_signal< msg_start >();
			}

		virtual void
		evt_shutdown()
			{
				++m_agents_finished;
				if( m_agents_finished == m_cfg.m_msg_types )
					{
						auto messages =
								static_cast< unsigned long long >( m_cfg.m_agents ) *
								m_cfg.m_mboxes *
								m_cfg.m_msg_types *
								m_cfg.m_iterations;

						m_benchmark.finish_and_show_stats( messages, "messages" );

						so_environment().stop();
					}
			}

	private :
		const so_5::subscription_storage_factory_t m_subscr_storage_factory;

		const so_5::mbox_t m_common_mbox;

		const cfg_t m_cfg;

		std::size_t m_agents_finished;

		benchmarker_t m_benchmark;

		std::vector< so_5::mbox_t > m_mboxes;
		std::vector< a_worker_t * > m_workers;

		typedef std::function< so_5::agent_t *() >
			sender_factory_t;

		std::vector< sender_factory_t > m_sender_factories;

		void
		create_sender_factories()
			{
				m_sender_factories.reserve( m_cfg.m_msg_types );

#define MAKE_SENDER_FACTORY(I)\
				m_sender_factories.emplace_back( \
						[this]() \
						{ \
							return new a_sender_t< msg_signal_##I >( so_environment(),  \
									m_subscr_storage_factory, \
									m_common_mbox, \
									m_cfg.m_iterations, \
									m_mboxes, \
									m_workers ); \
						} )

				MAKE_SENDER_FACTORY(0);
				MAKE_SENDER_FACTORY(1);
				MAKE_SENDER_FACTORY(2);
				MAKE_SENDER_FACTORY(3);
				MAKE_SENDER_FACTORY(4);
				MAKE_SENDER_FACTORY(5);
				MAKE_SENDER_FACTORY(6);
				MAKE_SENDER_FACTORY(7);
				MAKE_SENDER_FACTORY(8);
				MAKE_SENDER_FACTORY(9);
				MAKE_SENDER_FACTORY(10);
				MAKE_SENDER_FACTORY(11);
				MAKE_SENDER_FACTORY(12);
				MAKE_SENDER_FACTORY(13);
				MAKE_SENDER_FACTORY(14);
				MAKE_SENDER_FACTORY(15);
				MAKE_SENDER_FACTORY(16);
				MAKE_SENDER_FACTORY(17);
				MAKE_SENDER_FACTORY(18);
				MAKE_SENDER_FACTORY(19);
				MAKE_SENDER_FACTORY(20);
				MAKE_SENDER_FACTORY(21);
				MAKE_SENDER_FACTORY(22);
				MAKE_SENDER_FACTORY(23);
				MAKE_SENDER_FACTORY(24);
				MAKE_SENDER_FACTORY(25);
				MAKE_SENDER_FACTORY(26);
				MAKE_SENDER_FACTORY(27);
				MAKE_SENDER_FACTORY(28);
				MAKE_SENDER_FACTORY(29);
				MAKE_SENDER_FACTORY(30);
				MAKE_SENDER_FACTORY(31);

#undef MAKE_SENDER_FACTORY
			}

		void
		create_child_coop()
			{
				std::cout << "creating child coop..." << std::endl;

				{
					duration_meter_t meter( "creating mboxes" );
					m_mboxes.reserve( m_cfg.m_mboxes );
					for( std::size_t i = 0; i != m_cfg.m_mboxes; ++i )
						m_mboxes.emplace_back( so_environment().create_mbox() );
				}

				auto coop = so_environment().create_coop( "child" );
				coop->set_parent_coop_name( so_coop_name() );
				
				{
					duration_meter_t meter( "creating workers" );
					m_workers.reserve( m_cfg.m_agents );
					for( std::size_t i = 0; i != m_cfg.m_agents; ++i )
						{
							m_workers.push_back( new a_worker_t(
									so_environment(),
									m_subscr_storage_factory ) );
							coop->add_agent( m_workers.back() );
						}
				}

				{
					duration_meter_t meter( "creating senders and subscribe workers" );
					for( std::size_t i = 0; i != m_cfg.m_msg_types; ++i )
						{
							std::unique_ptr< so_5::agent_t > sender(
									m_sender_factories[i]() );
							coop->add_agent( std::move( sender ) );
						}
				}

				so_environment().register_coop( std::move( coop ) );

				std::cout << "child coop created..." << std::endl;
			}
	};

so_5::subscription_storage_factory_t
factory_by_cfg( const cfg_t & cfg )
	{
		using namespace so_5;

		const auto type = cfg.m_subscr_storage;
		if( subscr_storage_type_t::vector_based == type  )
			return vector_based_subscription_storage_factory(
					cfg.m_vector_subscr_storage_capacity );
		else if( subscr_storage_type_t::map_based == type )
			return map_based_subscription_storage_factory();
		else
			return hash_table_based_subscription_storage_factory();
	}

int
main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = try_parse_cmdline( argc, argv );
		if( cfg.m_msg_types > a_starter_stopper_t::max_msg_types )
			{
				std::ostringstream ss;
				ss << "too many msg_types specified: "
					<< cfg.m_msg_types
					<< ", max avaliable msg_types: "
					<< a_starter_stopper_t::max_msg_types;

				throw std::logic_error( ss.str() );
			}

		so_5::launch(
			[cfg]( so_5::environment_t & env )
			{
				env.register_agent_as_coop( "test",
						new a_starter_stopper_t(
								env,
								factory_by_cfg( cfg ),
								cfg ) );
			},
			[]( so_5::environment_params_t & params )
			{
				// This timer thread doesn't consume resources without
				// actual delayed/periodic messages.
				params.timer_thread( so_5::timer_list_factory() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

