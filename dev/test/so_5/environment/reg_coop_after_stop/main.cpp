/*
 * A test for checking autoshutdown during execution of init function.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <sstream>
#include <mutex>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class log_t
{
	public :
		void
		append( const std::string & what )
		{
			std::lock_guard< std::mutex > l( m_lock );
			m_value += what;
		}

		std::string
		get()
		{
			std::lock_guard< std::mutex > l( m_lock );
			return m_value;
		}

	private :
		std::mutex m_lock;
		std::string m_value;
};

class a_second_t : public so_5::agent_t
{
		struct msg_timer : public so_5::signal_t {};

	public :
		a_second_t(
			so_5::environment_t & env,
			log_t & log )
			:	so_5::agent_t( env )
			,	m_log( log )
		{}

		virtual void
		so_evt_start() override
		{
			m_log.append( "s.start;" );

			std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
		}

		virtual void
		so_evt_finish() override
		{
			m_log.append( "s.finish;" );
		}

	private :
		log_t & m_log;
};

class a_first_t : public so_5::agent_t
{
	public :
		a_first_t(
			so_5::environment_t & env,
			log_t & log )
			:	so_5::agent_t( env )
			,	m_log( log )
		{}

		virtual void
		so_evt_start() override
		{
			m_log.append( "f.start;" );

			std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );

			so_environment().stop();
			m_log.append( "env.stop;" );

			try
			{
				so_environment().register_agent_as_coop(
						"second",
						new a_second_t( so_environment(), m_log ),
						so_5::disp::active_obj::create_disp_binder( "active_obj" ) );
			}
			catch( const so_5::exception_t & x )
			{
				std::ostringstream s;
				s << "exception(" << x.error_code() << ");";
				m_log.append( s.str() );
			}
		}

		virtual void
		so_evt_finish() override
		{
			m_log.append( "f.finish;" );
		}

	private :
		log_t & m_log;
};

int
main()
{
	try
	{
		log_t log;

		run_with_time_limit(
			[&log]()
			{
				so_5::launch(
					[&log]( so_5::environment_t & env )
					{
						
						env.add_dispatcher_if_not_exists(
							"active_obj",
							[] { return so_5::disp::active_obj::create_disp(); } );

						env.register_agent_as_coop(
								"first",
								new a_first_t( env, log ) );
					} );
			},
			20,
			"SO Environment reg_coop_after_stop test" );

		const std::string expected = "f.start;env.stop;exception(28);f.finish;";
		const std::string actual = log.get();
		if( expected != actual )
			throw std::runtime_error( expected + " != " + actual );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

