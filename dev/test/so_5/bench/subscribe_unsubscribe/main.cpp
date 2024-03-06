/*
 * A benchmark for subscription/unsubscription operations.
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

//FIXME: does it really needed?
#if defined(__clang__) && (__clang_major__ >= 16)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif

namespace benchmark
{

enum class subscr_storage_type_t
	{
		vector_based,
		map_based,
		hash_table_based,
		flat_set_based
	};

const char *
subscr_storage_name( subscr_storage_type_t type )
	{
		if( subscr_storage_type_t::vector_based == type )
			return "vector_based";
		else if( subscr_storage_type_t::map_based == type )
			return "map_based";
		else if( subscr_storage_type_t::hash_table_based == type )
			return "hash_table_based";
		else
			return "flat_set_based";
	}

struct cfg_t
	{
		std::size_t m_agents = 32;
		std::size_t m_iterations = 10;
		std::size_t m_loops = 20;

		subscr_storage_type_t m_subscr_storage =
				subscr_storage_type_t::map_based;
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
							"_test.bench.so_5.subscribe_unsubscribe <options>\n"
							"\noptions:\n"
							"-a, --agents           count of agents\n"
							"-i, --iterations       count of iterations for subscribe/unsubscribe\n"
							"                       operations for every agent\n"
							"-l, --loops            loops to be done\n"
							"-s, --storage-type     type of subscription storage\n"
							"                       allowed values: vector, map, hash, flat_set\n"
							"-h, --help             show this description\n"
							<< std::endl;
					std::exit(1);
				}
			else if( is_arg( *current, "-a", "--agents" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_agents, ++current, last,
						"-a", "count of agents" );
			else if( is_arg( *current, "-i", "--iterations" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_iterations, ++current, last,
						"-i", "count of iterations for subscribe/unsubscribe" );
			else if( is_arg( *current, "-l", "--loops" ) )
				mandatory_arg_to_value(
						tmp_cfg.m_loops, ++current, last,
						"-l", "loops to be done" );
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
					else if( "flat_set" == type )
						tmp_cfg.m_subscr_storage = subscr_storage_type_t::flat_set_based;
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

// Types to be used for subscription/unsubscription operations.
struct olivia_msg final : public so_5::signal_t {};
struct emma_msg final : public so_5::signal_t {};
struct charlotte_msg final : public so_5::signal_t {};
struct amelia_msg final : public so_5::signal_t {};
struct sophia_msg final : public so_5::signal_t {};
struct isabella_msg final : public so_5::signal_t {};
struct ava_msg final : public so_5::signal_t {};
struct mia_msg final : public so_5::signal_t {};
struct evelyn_msg final : public so_5::signal_t {};
struct luna_msg final : public so_5::signal_t {};
struct harper_msg final : public so_5::signal_t {};
struct camila_msg final : public so_5::signal_t {};
struct sofia_msg final : public so_5::signal_t {};
struct scarlett_msg final : public so_5::signal_t {};
struct elizabeth_msg final : public so_5::signal_t {};
struct eleanor_msg final : public so_5::signal_t {};
struct emily_msg final : public so_5::signal_t {};
struct chloe_msg final : public so_5::signal_t {};
struct mila_msg final : public so_5::signal_t {};
struct violet_msg final : public so_5::signal_t {};
struct penelope_msg final : public so_5::signal_t {};
struct gianna_msg final : public so_5::signal_t {};
struct aria_msg final : public so_5::signal_t {};
struct abigail_msg final : public so_5::signal_t {};
struct ella_msg final : public so_5::signal_t {};
struct avery_msg final : public so_5::signal_t {};
struct hazel_msg final : public so_5::signal_t {};
struct nora_msg final : public so_5::signal_t {};
struct layla_msg final : public so_5::signal_t {};
struct lily_msg final : public so_5::signal_t {};
struct aurora_msg final : public so_5::signal_t {};
struct nova_msg final : public so_5::signal_t {};
struct ellie_msg final : public so_5::signal_t {};
struct madison_msg final : public so_5::signal_t {};
struct grace_msg final : public so_5::signal_t {};
struct isla_msg final : public so_5::signal_t {};
struct willow_msg final : public so_5::signal_t {};
struct zoe_msg final : public so_5::signal_t {};
struct riley_msg final : public so_5::signal_t {};
struct stella_msg final : public so_5::signal_t {};
struct eliana_msg final : public so_5::signal_t {};
struct ivy_msg final : public so_5::signal_t {};
struct victoria_msg final : public so_5::signal_t {};
struct emilia_msg final : public so_5::signal_t {};
struct zoey_msg final : public so_5::signal_t {};
struct naomi_msg final : public so_5::signal_t {};
struct hannah_msg final : public so_5::signal_t {};
struct lucy_msg final : public so_5::signal_t {};
struct elena_msg final : public so_5::signal_t {};
struct lillian_msg final : public so_5::signal_t {};
struct maya_msg final : public so_5::signal_t {};
struct leah_msg final : public so_5::signal_t {};
struct paisley_msg final : public so_5::signal_t {};
struct addison_msg final : public so_5::signal_t {};
struct natalie_msg final : public so_5::signal_t {};
struct valentina_msg final : public so_5::signal_t {};
struct everly_msg final : public so_5::signal_t {};
struct delilah_msg final : public so_5::signal_t {};
struct leilani_msg final : public so_5::signal_t {};
struct madelyn_msg final : public so_5::signal_t {};
struct kinsley_msg final : public so_5::signal_t {};
struct ruby_msg final : public so_5::signal_t {};
struct sophie_msg final : public so_5::signal_t {};
struct alice_msg final : public so_5::signal_t {};

struct msg_next_loop : public so_5::signal_t {};

class a_worker_t
	:	public so_5::agent_t
	{
	public :
		a_worker_t(
			context_t ctx,
			std::size_t iterations,
			so_5::subscription_storage_factory_t subscr_storage_factory )
			:	so_5::agent_t{ ctx + subscr_storage_factory }
			,	m_iterations{ iterations }
			{}

		void
		set_next( so_5::mbox_t next )
			{
				m_next = std::move(next);
			}

		void
		so_define_agent()
			{
				so_subscribe_self().event( &a_worker_t::evt_next_loop );
			}

	protected :
		const std::size_t m_iterations;

		so_5::mbox_t m_next;

		template< typename Signal >
		void
		make_subscription()
			{
				so_subscribe_self().event( [this](mhood_t<Signal>) {
						/* Nothing to do */
					} );
			}

		template< typename Signal >
		void
		drop_subscription()
			{
				so_drop_subscription<Signal>( so_direct_mbox() );
			}

		void
		evt_next_loop( mhood_t< msg_next_loop > )
			{
				for( std::size_t i = 0; i != m_iterations; ++i )
				{
					// Subscription part.
					make_subscription< olivia_msg >();
					make_subscription< emma_msg >();
					make_subscription< charlotte_msg >();
					make_subscription< amelia_msg >();
					make_subscription< sophia_msg >();
					make_subscription< isabella_msg >();
					make_subscription< ava_msg >();
					make_subscription< mia_msg >();
					make_subscription< evelyn_msg >();
					make_subscription< luna_msg >();
					make_subscription< harper_msg >();
					make_subscription< camila_msg >();
					make_subscription< sofia_msg >();
					make_subscription< scarlett_msg >();
					make_subscription< elizabeth_msg >();
					make_subscription< eleanor_msg >();
					make_subscription< emily_msg >();
					make_subscription< chloe_msg >();
					make_subscription< mila_msg >();
					make_subscription< violet_msg >();
					make_subscription< penelope_msg >();
					make_subscription< gianna_msg >();
					make_subscription< aria_msg >();
					make_subscription< abigail_msg >();
					make_subscription< ella_msg >();
					make_subscription< avery_msg >();
					make_subscription< hazel_msg >();
					make_subscription< nora_msg >();
					make_subscription< layla_msg >();
					make_subscription< lily_msg >();
					make_subscription< aurora_msg >();
					make_subscription< nova_msg >();
					make_subscription< ellie_msg >();
					make_subscription< madison_msg >();
					make_subscription< grace_msg >();
					make_subscription< isla_msg >();
					make_subscription< willow_msg >();
					make_subscription< zoe_msg >();
					make_subscription< riley_msg >();
					make_subscription< stella_msg >();
					make_subscription< eliana_msg >();
					make_subscription< ivy_msg >();
					make_subscription< victoria_msg >();
					make_subscription< emilia_msg >();
					make_subscription< zoey_msg >();
					make_subscription< naomi_msg >();
					make_subscription< hannah_msg >();
					make_subscription< lucy_msg >();
					make_subscription< elena_msg >();
					make_subscription< lillian_msg >();
					make_subscription< maya_msg >();
					make_subscription< leah_msg >();
					make_subscription< paisley_msg >();
					make_subscription< addison_msg >();
					make_subscription< natalie_msg >();
					make_subscription< valentina_msg >();
					make_subscription< everly_msg >();
					make_subscription< delilah_msg >();
					make_subscription< leilani_msg >();
					make_subscription< madelyn_msg >();
					make_subscription< kinsley_msg >();
					make_subscription< ruby_msg >();
					make_subscription< sophie_msg >();
					make_subscription< alice_msg >();

					// Unsubscription part.
					drop_subscription< zoey_msg >();
					drop_subscription< hannah_msg >();
					drop_subscription< lillian_msg >();
					drop_subscription< eleanor_msg >();
					drop_subscription< madelyn_msg >();
					drop_subscription< leilani_msg >();
					drop_subscription< emma_msg >();
					drop_subscription< paisley_msg >();
					drop_subscription< madison_msg >();
					drop_subscription< violet_msg >();
					drop_subscription< alice_msg >();
					drop_subscription< maya_msg >();
					drop_subscription< ava_msg >();
					drop_subscription< riley_msg >();
					drop_subscription< eliana_msg >();
					drop_subscription< elizabeth_msg >();
					drop_subscription< charlotte_msg >();
					drop_subscription< elena_msg >();
					drop_subscription< gianna_msg >();
					drop_subscription< chloe_msg >();
					drop_subscription< mia_msg >();
					drop_subscription< victoria_msg >();
					drop_subscription< willow_msg >();
					drop_subscription< kinsley_msg >();
					drop_subscription< grace_msg >();
					drop_subscription< sophia_msg >();
					drop_subscription< mila_msg >();
					drop_subscription< sophie_msg >();
					drop_subscription< amelia_msg >();
					drop_subscription< isabella_msg >();
					drop_subscription< natalie_msg >();
					drop_subscription< everly_msg >();
					drop_subscription< emilia_msg >();
					drop_subscription< layla_msg >();
					drop_subscription< naomi_msg >();
					drop_subscription< ruby_msg >();
					drop_subscription< lucy_msg >();
					drop_subscription< sofia_msg >();
					drop_subscription< stella_msg >();
					drop_subscription< nora_msg >();
					drop_subscription< penelope_msg >();
					drop_subscription< camila_msg >();
					drop_subscription< ella_msg >();
					drop_subscription< aria_msg >();
					drop_subscription< ivy_msg >();
					drop_subscription< aurora_msg >();
					drop_subscription< ellie_msg >();
					drop_subscription< emily_msg >();
					drop_subscription< leah_msg >();
					drop_subscription< zoe_msg >();
					drop_subscription< valentina_msg >();
					drop_subscription< isla_msg >();
					drop_subscription< harper_msg >();
					drop_subscription< avery_msg >();
					drop_subscription< nova_msg >();
					drop_subscription< abigail_msg >();
					drop_subscription< hazel_msg >();
					drop_subscription< evelyn_msg >();
					drop_subscription< olivia_msg >();
					drop_subscription< addison_msg >();
					drop_subscription< lily_msg >();
					drop_subscription< scarlett_msg >();
					drop_subscription< luna_msg >();
					drop_subscription< delilah_msg >();
				}

				so_5::send< msg_next_loop >( m_next );
			}
	};

class a_first_worker_t final : public a_worker_t
	{
	public:
		a_first_worker_t(
			context_t ctx,
			std::size_t loops,
			std::size_t iterations,
			so_5::subscription_storage_factory_t subscr_storage_factory )
			:	a_worker_t{ std::move(ctx), iterations, std::move(subscr_storage_factory) }
			,	m_loops{ loops }
			{}

		void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( &a_first_worker_t::evt_custom_next_loop )
					;
			}

		void
		so_evt_start() override
			{
				so_5::send< msg_next_loop >( *this );
			}

	private:
		const std::size_t m_loops;
		std::size_t m_loop_index{};

		benchmarker_t m_benchmark;

		void
		evt_custom_next_loop( mhood_t< msg_next_loop > cmd )
			{
				const bool do_actions = [this, &cmd]() {
						if( 0u == m_loop_index )
						{
							// Start of the benchmark.
							m_benchmark.start();
							return true;
						}
						else if( m_loops == m_loop_index )
						{
							// Benchmark has to be completed.
							m_benchmark.finish_and_show_stats(
									static_cast<unsigned long long>( m_loops ) * m_iterations,
									"iterations" );
							so_deregister_agent_coop_normally();

							return false;
						}
						else
							return true;
					}();

				if( do_actions )
				{
					++m_loop_index;

					// Let the base class do the main work.
					evt_next_loop( cmd );
				}
			}
	};

so_5::subscription_storage_factory_t
factory_by_cfg( const cfg_t & cfg )
	{
		using namespace so_5;

		constexpr std::size_t default_initial_capacity = 65;

		const auto type = cfg.m_subscr_storage;
		if( subscr_storage_type_t::vector_based == type  )
			return vector_based_subscription_storage_factory(
					default_initial_capacity );
		else if( subscr_storage_type_t::map_based == type )
			return map_based_subscription_storage_factory();
		else if( subscr_storage_type_t::hash_table_based == type )
			return hash_table_based_subscription_storage_factory();
		else
			return flat_set_based_subscription_storage_factory(
					default_initial_capacity );
	}

void
run_benchmark(
	so_5::environment_t & env,
	const cfg_t & cfg )
{
	env.introduce_coop( [&cfg]( so_5::coop_t & coop ) {
			auto factory = factory_by_cfg( cfg );

			std::vector< a_worker_t * > workers;
			workers.reserve( cfg.m_agents );

			workers.push_back( coop.make_agent< a_first_worker_t >(
					cfg.m_loops,
					cfg.m_iterations,
					factory ) );

			for( std::size_t i = 1; i != cfg.m_agents; ++i )
				workers.push_back( coop.make_agent< a_worker_t >(
						cfg.m_iterations,
						factory ) );

			for( std::size_t i = 0; i != cfg.m_agents; ++i )
				workers[ i ]->set_next( workers[ (i + 1) % cfg.m_agents ]->so_direct_mbox() );
		} );
}

} /* namespace benchmark */

using namespace benchmark;

int
main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = try_parse_cmdline( argc, argv );
		if( cfg.m_agents < 2u )
			{
				std::ostringstream ss;
				ss << "number of agents is too small: " << cfg.m_agents;

				throw std::logic_error( ss.str() );
			}

		std::cout
				<< "* agents: " << cfg.m_agents << "\n"
				<< "* iterations: " << cfg.m_iterations << "\n"
				<< "* loops: " << cfg.m_loops << "\n"
				<< "* subscr_storage: " << subscr_storage_name( cfg.m_subscr_storage )
				<< std::endl;

		so_5::launch(
			[cfg]( so_5::environment_t & env )
			{
				run_benchmark( env, cfg );
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

