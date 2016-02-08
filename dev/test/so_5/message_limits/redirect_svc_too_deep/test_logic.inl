/*
 * A simple test for message limits (redirecting service request).
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_request : public so_5::rt::signal_t {};

class a_worker_t : public so_5::rt::agent_t
{
public :
	a_worker_t(
		so_5::rt::environment_t & env,
		std::string reply )
		:	so_5::rt::agent_t( env +
				limit_then_redirect< msg_request >( 2,
					[this] { return m_self_mbox; } ) )
		,	m_reply( std::move( reply ) )
	{}

	void
	set_self_mbox( const so_5::rt::mbox_t & mbox )
	{
		m_self_mbox = mbox;
	}

	virtual void
	so_define_agent() override
	{
		so_default_state().event< msg_request >( m_self_mbox,
			[&]() -> std::string { return m_reply; } );
	}

private :
	so_5::rt::mbox_t m_self_mbox;
	const std::string m_reply;
};

class a_manager_t : public so_5::rt::agent_t
{
public :
	a_manager_t(
		so_5::rt::environment_t & env,
		std::string expected_response )
		:	so_5::rt::agent_t( env )
		,	m_expected_response( expected_response )
	{}

	void
	set_self_mbox( const so_5::rt::mbox_t & mbox )
	{
		m_self_mbox = mbox;
	}

	void
	set_target_mbox( const so_5::rt::mbox_t & mbox )
	{
		m_target_mbox = mbox;
	}

	virtual void
	so_define_agent() override
	{
		so_default_state().event< msg_get_results >(
			[&] {
				std::string responses = m_r1.get() + m_r2.get();
				
				try
				{
					responses += m_r3.get();
					throw std::runtime_error( "Exception from SObjectizer expected!" );
				}
				catch( const std::future_error & )
				{
					// This is an expected exception.
				}

				if( m_expected_response == responses )
					so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		auto svc = m_target_mbox->get_one< std::string >();

		m_r1 = svc.async< msg_request >();
		m_r2 = svc.async< msg_request >();
		m_r3 = svc.async< msg_request >();

		so_5::send_to_agent< msg_get_results >( *this );
	}

private :
	struct msg_get_results : public so_5::rt::signal_t {};

	so_5::rt::mbox_t m_self_mbox;
	so_5::rt::mbox_t m_target_mbox;

	const std::string m_expected_response;

	std::future< std::string > m_r1;
	std::future< std::string > m_r2;
	std::future< std::string > m_r3;
};

void
do_test(
	const std::string & test_name,
	std::function< void(a_manager_t &, a_worker_t &) > test_tuner )
{
	try
	{
		run_with_time_limit(
			[test_tuner]()
			{
				so_5::launch(
						[test_tuner]( so_5::rt::environment_t & env )
						{
							auto coop = env.create_coop( so_5::autoname );
							
							auto manager = coop->make_agent< a_manager_t >(
									"[one][one]" );

							auto worker = coop->make_agent< a_worker_t >( "[one]" );

							test_tuner( *manager, *worker  );

							env.register_coop( std::move( coop ) );
						} );
			},
			20,
			test_name );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		std::abort();
	}
}
