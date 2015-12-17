/*
 * A sample for an exception from service handler demonstration.
 */

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <chrono>

#include <so_5/all.hpp>

so_5::mbox_t make_converter( so_5::coop_t & coop )
	{
		auto a = coop.define_agent();
		a.event( a, []( const std::string & value ) -> int {
				std::istringstream s( value );
				int result;
				s >> result;
				if( s.fail() )
					throw std::invalid_argument( "unable to convert to int: '" + value + "'" );

				// A special case for timeout imitation.
				if( 42 == result )
					std::this_thread::sleep_for( std::chrono::milliseconds(150) );

				return result;
			} );

		return a.direct_mbox();
	}

class a_client_t : public so_5::agent_t
	{
	public :
		a_client_t( context_t ctx, so_5::mbox_t svc_mbox )
			:	so_5::agent_t( ctx )
			,	m_svc_mbox( std::move(svc_mbox) )
			{}

		virtual void so_evt_start() override
			{
				std::string values_to_convert[] = {
						"1", "2", "a1", "a2", "3", "a3", "41", "42", "43" };

				for( const auto & s : values_to_convert )
					{
						try
							{
								std::cout << "converting '" << s << "'" << std::flush;

								auto r = so_5::request_value< int, std::string >(
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
		const so_5::mbox_t m_svc_mbox;
	};

int main()
	{
		try
			{
				so_5::launch( []( so_5::environment_t & env ) {
						env.introduce_coop(
								so_5::disp::active_obj::create_private_disp( env )->binder(),
								[]( so_5::coop_t & coop ) {
									auto service = make_converter( coop );
									coop.make_agent< a_client_t >( service );
								} );
					} );
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}

