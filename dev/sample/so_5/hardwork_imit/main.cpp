/*
 * Sample for demonstrating results of some hard work on different
 * dispatchers.
 */

#include <iostream>
#include <chrono>
#include <cstdlib>

#include <so_5/all.hpp>

struct msg_do_hardwork
{
	unsigned int m_index;
	unsigned int m_milliseconds;
};

struct msg_hardwork_done
{
	unsigned int m_index;
};

struct msg_check_hardwork
{
	unsigned int m_index;
	unsigned int m_milliseconds;
};

struct msg_hardwork_checked
{
	unsigned int m_index;
};

class a_manager_t final : public so_5::agent_t
{
	public :
		a_manager_t(
			context_t ctx,
			so_5::mbox_t worker_mbox,
			so_5::mbox_t checker_mbox,
			unsigned int requests,
			unsigned int milliseconds )
			:	so_5::agent_t( ctx )
			,	m_worker_mbox( std::move(worker_mbox) )
			,	m_checker_mbox( std::move(checker_mbox) )
			,	m_requests( requests )
			,	m_milliseconds( milliseconds )
		{}

		void so_define_agent() override
		{
			so_subscribe_self()
				.event( &a_manager_t::evt_hardwork_done )
				.event( &a_manager_t::evt_hardwork_checked );
		}

		void so_evt_start() override
		{
			m_start_time = std::chrono::steady_clock::now();

			for( unsigned int i = 0; i != m_requests; ++i )
			{
				so_5::send< msg_do_hardwork >(
						m_worker_mbox,
						i, m_milliseconds );
			}
		}

		void evt_hardwork_done( const msg_hardwork_done & evt )
		{
			so_5::send< msg_check_hardwork >(
					m_checker_mbox,
					evt.m_index, m_milliseconds );
		}

		void evt_hardwork_checked( const msg_hardwork_checked & )
		{
			++m_processed;

			if( m_processed == m_requests )
			{
				auto finish_time = std::chrono::steady_clock::now();

				auto duration = double(
						std::chrono::duration_cast< std::chrono::milliseconds >(
								finish_time - m_start_time ).count()
						) / 1000.0;

				std::cout << "Working time: " << duration << "s" << std::endl;

				so_environment().stop();
			}
		}

	private :
		const so_5::mbox_t m_worker_mbox;
		const so_5::mbox_t m_checker_mbox;

		const unsigned int m_requests;
		unsigned int m_processed = 0;

		const unsigned int m_milliseconds;

		std::chrono::steady_clock::time_point m_start_time;
};

so_5::coop_unique_holder_t
create_test_coop(
	so_5::environment_t & env,
	so_5::disp_binder_shptr_t disp_binder,
	unsigned int requests,
	unsigned int milliseconds )
{
	class worker_t final : public so_5::agent_t {
	public :
		using so_5::agent_t::agent_t;

		void bind_to( const so_5::agent_t & manager ) {
			so_subscribe_self().event(
				[&manager]( mhood_t<msg_do_hardwork> cmd ) {
					std::this_thread::sleep_for(
							std::chrono::milliseconds( cmd->m_milliseconds ) );

					so_5::send< msg_hardwork_done >( manager, cmd->m_index );
				},
				so_5::thread_safe );
		}
	};

	class checker_t final : public so_5::agent_t {
	public :
		using so_5::agent_t::agent_t;

		void bind_to( const so_5::agent_t & manager ) {
			so_subscribe_self().event(
				[&manager]( mhood_t<msg_check_hardwork> cmd ) {
					std::this_thread::sleep_for(
							std::chrono::milliseconds( cmd->m_milliseconds ) );

					so_5::send< msg_hardwork_checked >( manager, cmd->m_index );
				},
				so_5::thread_safe );
		}
	};

	auto c = env.make_coop( std::move( disp_binder ) );

	auto worker = c->make_agent< worker_t >();
	auto checker = c->make_agent< checker_t >();

	auto manager = c->make_agent< a_manager_t >(
			worker->so_direct_mbox(),
			checker->so_direct_mbox(),
			requests,
			milliseconds );

	worker->bind_to( *manager );
	checker->bind_to( *manager );

	return c;
}

using dispatcher_factory_t = std::function<
		so_5::disp_binder_shptr_t( so_5::environment_t & ) >;

dispatcher_factory_t
make_dispatcher_factory( std::string_view type )
{
	dispatcher_factory_t res;

	if( "active_obj" == type )
	{
		using namespace so_5::disp::active_obj;
		res = []( so_5::environment_t & env ) {
			return make_dispatcher( env ).binder();
		};
	}
	else if( "thread_pool" == type )
	{
		using namespace so_5::disp::thread_pool;
		res = []( so_5::environment_t & env ) {
			return make_dispatcher( env ).binder(
					[]( bind_params_t & p ) { p.fifo( fifo_t::individual ); } );
		};
	}
	else if( "adv_thread_pool" == type )
	{
		using namespace so_5::disp::adv_thread_pool;
		res = []( so_5::environment_t & env ) {
			return make_dispatcher( env ).binder(
					[]( bind_params_t & p ) { p.fifo( fifo_t::individual ); } );
		};
	}
	else if( "one_thread" == type )
	{
		using namespace so_5::disp::one_thread;
		res = []( so_5::environment_t & env ) {
			return make_dispatcher( env ).binder();
		};
	}
	else
		throw std::runtime_error(
				"unknown type of dispatcher: " + std::string{ type } );


	return res;
}

struct config_t
{
	dispatcher_factory_t m_factory;
	unsigned int m_requests;
	unsigned int m_milliseconds;
};

config_t
parse_params( int argc, char ** argv )
{
	if( 1 == argc )
		throw std::runtime_error( "no arguments given!\n\n"
				"usage:\n\n"
				"sample.so_5.hardwork_imit <disp_type> [requests] [worktime_ms]" );

	config_t r {
			make_dispatcher_factory( argv[ 1 ] ),
			200,
			15
		};

	if( 2 < argc )
		r.m_requests = static_cast< unsigned int >( std::atoi( argv[ 2 ] ) );
	if( 3 < argc )
		r.m_milliseconds = static_cast< unsigned int >( std::atoi( argv[ 3 ] ) );

	std::cout << "Config:\n"
		"\t" "dispatcher: " << argv[ 1 ] << "\n"
		"\t" "requests: " << r.m_requests << "\n"
		"\t" "worktime (ms): " << r.m_milliseconds << std::endl;

	return r;
}

int main( int argc, char ** argv )
{
	try
	{
		const config_t config = parse_params( argc, argv );

		so_5::launch(
			[&config]( so_5::environment_t & env )
			{
				env.register_coop(
						create_test_coop(
								env,
								config.m_factory( env ),
								config.m_requests,
								config.m_milliseconds ) );
			} );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}

