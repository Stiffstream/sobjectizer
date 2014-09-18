/*
 * A simple service handler test.
 */

#include <iostream>
#include <exception>
#include <sstream>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>
#include <so_5/h/types.hpp>

#include <so_5/disp/active_obj/h/pub.hpp>

#include "../a_time_sentinel.hpp"

struct msg_convert : public so_5::rt::message_t
	{
		int m_value;

		msg_convert( int value ) : m_value( value )
			{}
	};

class a_convert_service_t
	:	public so_5::rt::agent_t
	{
	public :
		a_convert_service_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & self_mbox )
			:	so_5::rt::agent_t( env )
			,	m_self_mbox( self_mbox )
			{}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_self_mbox )
						.event( &a_convert_service_t::svc_convert );
			}

		std::string
		svc_convert( const so_5::rt::event_data_t< msg_convert > & evt )
			{
				std::ostringstream s;
				s << evt->m_value;

				return s.str();
			}

	private :
		const so_5::rt::mbox_ref_t m_self_mbox;
	};

struct msg_complex_svc : public so_5::rt::message_t
	{
		int m_i;
		std::string m_s;
		std::unique_ptr< std::vector< int > > m_v;
		const long long & m_r;

		msg_complex_svc(
			int i,
			std::string && s,
			std::unique_ptr< std::vector< int > > v,
			const long long & r )
			:	m_i( i )
			,	m_s( s )
			,	m_v( std::move( v ) )
			,	m_r( r )
			{}
	};

class a_complex_service_t
	:	public so_5::rt::agent_t
	{
	public :
		a_complex_service_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & self_mbox )
			:	so_5::rt::agent_t( env )
			,	m_self_mbox( self_mbox )
			{}

		virtual void
		so_define_agent()
			{
				so_subscribe( m_self_mbox )
						.event( &a_complex_service_t::svc );
			}

		void
		svc( const so_5::rt::event_data_t< msg_complex_svc > & evt )
			{
			}

	private :
		const so_5::rt::mbox_ref_t m_self_mbox;
	};

class a_client_t
	:	public so_5::rt::agent_t
	{
	public :
		a_client_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & svc_mbox )
			:	so_5::rt::agent_t( env )
			,	m_svc_mbox( svc_mbox )
			{}

		virtual void
		so_evt_start()
			{
				m_svc_mbox->get_one< std::string >()
						.wait_forever().make_sync_get< msg_convert >( 1 );

				long long l = 10;
				m_svc_mbox->get_one< void >()
						.wait_forever().make_sync_get< msg_complex_svc >(
								1,
								"Hello, World",
								std::unique_ptr< std::vector< int > >(
										new std::vector< int >( 100 ) ),
								l );

				so_environment().stop();
			}

	private :
		const so_5::rt::mbox_ref_t m_svc_mbox;
	};

void
init(
	so_5::rt::so_environment_t & env )
	{
		auto coop = env.create_coop(
				so_5::rt::nonempty_name_t( "test_coop" ),
				so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

		auto svc_mbox = env.create_local_mbox();

		coop->add_agent( new a_convert_service_t( env, svc_mbox ) );
		coop->add_agent( new a_complex_service_t( env, svc_mbox ) );
		coop->add_agent( new a_client_t( env, svc_mbox ) );
		coop->add_agent( new a_time_sentinel_t( env ) );

		env.register_coop( std::move( coop ) );
	}

int
main( int, char ** )
	{
		try
			{
				so_5::api::run_so_environment(
					&init,
					std::move(
						so_5::rt::so_environment_params_t()
							.add_named_dispatcher(
								so_5::rt::nonempty_name_t( "active_obj" ),
								so_5::disp::active_obj::create_disp() ) ) );
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}

