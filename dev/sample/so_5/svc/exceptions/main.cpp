/*
 * A sample for an exception from service handler demonstration.
 */

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <chrono>

#include <so_5/all.hpp>

struct msg_convert : public so_5::rt::message_t
	{
		const std::string m_value;

		msg_convert( const std::string & value ) : m_value( value )
			{}
	};

class a_convert_service_t
	:	public so_5::rt::agent_t
	{
	public :
		a_convert_service_t(
			so_5::rt::environment_t & env )
			:	so_5::rt::agent_t( env )
			{}

		virtual void
		so_define_agent() override
			{
				so_subscribe_self()
						.event( []( const msg_convert & msg ) -> int
						{
							std::istringstream s( msg.m_value );
							int result;
							s >> result;
							if( s.fail() )
								throw std::invalid_argument( "unable to convert to int: '" +
										msg.m_value + "'" );

							// A special case for timeout imitation.
							if( 42 == result )
								std::this_thread::sleep_for( std::chrono::milliseconds(150) );

							return result;
						} );
			}
	};

class a_client_t
	:	public so_5::rt::agent_t
	{
	public :
		a_client_t(
			so_5::rt::environment_t & env,
			const so_5::rt::mbox_t & svc_mbox )
			:	so_5::rt::agent_t( env )
			,	m_svc_mbox( svc_mbox )
			{}

		virtual void
		so_evt_start() override
			{
				std::string values_to_convert[] = {
						"1", "2", "a1", "a2", "3", "a3", "41", "42", "43" };

				for( const auto & s : values_to_convert )
					{
						try
							{
								std::cout << "converting '" << s << "'" << std::flush;

								auto r = so_5::request_value< int, msg_convert >(
										m_svc_mbox, std::chrono::milliseconds(100), s );

								std::cout << "' -> " << r << std::endl;
							}
						catch( const std::exception & x )
							{
								std::cerr << "\n*** an exception during converting "
										"value '" << s << "': " << x.what()
										<< std::endl;
							}
					}

				so_environment().stop();
			}

	private :
		const so_5::rt::mbox_t m_svc_mbox;
	};

void
init(
	so_5::rt::environment_t & env )
	{
		env.introduce_coop(
				so_5::disp::active_obj::create_private_disp( env )->binder(),
				[]( so_5::rt::coop_t & coop ) {
					auto a_service = coop.make_agent< a_convert_service_t >();
					coop.make_agent< a_client_t >( a_service->so_direct_mbox() );
				} );
	}

int
main()
	{
		try
			{
				so_5::launch( &init );
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}

