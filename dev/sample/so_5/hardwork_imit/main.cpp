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

class a_manager_t : public so_5::agent_t
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

		virtual void so_define_agent() override
		{
			so_subscribe_self()
				.event( &a_manager_t::evt_hardwork_done )
				.event( &a_manager_t::evt_hardwork_checked );
		}

		virtual void so_evt_start() override
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

				auto duration =
						std::chrono::duration_cast< std::chrono::milliseconds >(
								finish_time - m_start_time ).count() / 1000.0;

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

so_5::coop_unique_ptr_t
create_test_coop(
	so_5::environment_t & env,
	so_5::disp_binder_unique_ptr_t disp_binder,
	unsigned int requests,
	unsigned int milliseconds )
{
	auto c = env.create_coop( "test", std::move( disp_binder ) );

	auto worker = c->define_agent();
	auto checker = c->define_agent();

	auto a_manager = c->make_agent< a_manager_t >(
			worker.direct_mbox(),
			checker.direct_mbox(),
			requests,
			milliseconds );

	worker.event( worker.direct_mbox(),
				[a_manager]( const msg_do_hardwork & evt )
				{
					std::this_thread::sleep_for(
							std::chrono::milliseconds( evt.m_milliseconds ) );

					so_5::send< msg_hardwork_done >( *a_manager, evt.m_index );
				},
				so_5::thread_safe );

	checker.event( checker.direct_mbox(),
				[a_manager]( const msg_check_hardwork & evt )
				{
					std::this_thread::sleep_for(
							std::chrono::milliseconds( evt.m_milliseconds ) );

					so_5::send< msg_hardwork_checked >( *a_manager, evt.m_index );
				},
				so_5::thread_safe );

	return c;
}

struct dispatcher_factories_t
{
	std::function< so_5::dispatcher_unique_ptr_t() > m_disp_factory;
	std::function< so_5::disp_binder_unique_ptr_t() > m_binder_factory;
};

dispatcher_factories_t
make_dispatcher_factories(
	const std::string & type,
	const std::string & name )
{
	dispatcher_factories_t res;

	if( "active_obj" == type )
	{
		using namespace so_5::disp::active_obj;
		res.m_disp_factory = [] { return create_disp(); };
		res.m_binder_factory = [name]() { return create_disp_binder( name ); };
	}
	else if( "thread_pool" == type )
	{
		using namespace so_5::disp::thread_pool;
		res.m_disp_factory = []() { return create_disp(); };
		res.m_binder_factory = 
			[name]() {
				return create_disp_binder(
						name,
						[]( bind_params_t & p ) { p.fifo( fifo_t::individual ); } );
			};
	}
	else if( "adv_thread_pool" == type )
	{
		using namespace so_5::disp::adv_thread_pool;
		res.m_disp_factory = []() { return create_disp(); };
		res.m_binder_factory = 
			[name]() {
				return create_disp_binder(
						name,
						[]( bind_params_t & p ) { p.fifo( fifo_t::individual ); } );
			};
	}
	else if( "one_thread" == type )
	{
		using namespace so_5::disp::one_thread;

		res.m_disp_factory = [] { return create_disp(); };
		res.m_binder_factory = [name]() { return create_disp_binder( name ); };
	}
	else
		throw std::runtime_error( "unknown type of dispatcher: " + type );


	return res;
}

struct config_t
{
	dispatcher_factories_t m_factories;
	unsigned int m_requests;
	unsigned int m_milliseconds;

	static const std::string dispatcher_name;
};

const std::string config_t::dispatcher_name = "dispatcher";

config_t
parse_params( int argc, char ** argv )
{
	if( 1 == argc )
		throw std::runtime_error( "no arguments given!\n\n"
				"usage:\n\n"
				"sample.so_5.hardwork_imit <disp_type> [requests] [worktime_ms]" );

	config_t r {
			make_dispatcher_factories( argv[ 1 ], config_t::dispatcher_name ),
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
			[config]( so_5::environment_t & env )
			{
				env.register_coop(
						create_test_coop(
								env,
								config.m_factories.m_binder_factory(),
								config.m_requests,
								config.m_milliseconds ) );
			},
			[config]( so_5::environment_params_t & params )
			{
				params.add_named_dispatcher(
						config_t::dispatcher_name,
						config.m_factories.m_disp_factory() );
			} );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}

