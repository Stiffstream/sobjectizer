/*
 * A simple service handler test.
 */

#include <iostream>
#include <exception>
#include <sstream>

#include <so_5/all.hpp>

#include "../a_time_sentinel.hpp"

struct msg_convert : public so_5::message_t
	{
		int m_value;

		msg_convert( int value ) : m_value( value )
			{}
	};

class a_convert_service_t
	:	public so_5::agent_t
	{
	public :
		a_convert_service_t(
			so_5::environment_t & env,
			const so_5::mbox_t & self_mbox )
			:	so_5::agent_t( env )
			,	m_self_mbox( self_mbox )
			{}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_self_mbox )
						.event( &a_convert_service_t::svc_convert );
			}

		std::string
		svc_convert( const so_5::event_data_t< msg_convert > & evt )
			{
				std::ostringstream s;
				s << evt->m_value;

				return s.str();
			}

	private :
		const so_5::mbox_t m_self_mbox;
	};

class a_client_t
	:	public so_5::agent_t
	{
	public :
		a_client_t(
			so_5::environment_t & env,
			const so_5::mbox_t & svc_mbox )
			:	so_5::agent_t( env )
			,	m_svc_mbox( svc_mbox )
			{}

		virtual void
		so_evt_start()
			{
				try
					{
						m_svc_mbox->get_one< std::string >()
								.wait_forever()
								.sync_get( new msg_convert( 3 ) );

						std::cerr << "An exception rc_more_than_one_svc_handler expected"
								<< std::endl;

						std::abort();
					}
				catch( const so_5::exception_t & x )
					{
						if( so_5::rc_more_than_one_svc_handler != x.error_code() )
							{
								std::cerr << "Unexpected error_code: "
										<< x.error_code()
										<< ", expected: "
										<< so_5::rc_no_svc_handlers
										<< std::endl;

								std::abort();
							}
					}

				so_environment().stop();
			}

	private :
		const so_5::mbox_t m_svc_mbox;
	};

void
init(
	so_5::environment_t & env )
	{
		auto coop = env.create_coop(
				"test_coop",
				so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

		auto svc_mbox = env.create_mbox();

		coop->add_agent( new a_convert_service_t( env, svc_mbox ) );
		coop->add_agent( new a_convert_service_t( env, svc_mbox ) );
		coop->add_agent( new a_client_t( env, svc_mbox ) );
		coop->add_agent( new a_time_sentinel_t( env ) );

		env.register_coop( std::move( coop ) );
	}

int
main()
	{
		try
			{
				so_5::launch(
					&init,
					[]( so_5::environment_params_t & params )
					{
						params.add_named_dispatcher(
								"active_obj",
								so_5::disp::active_obj::create_disp() );
					} );
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}

