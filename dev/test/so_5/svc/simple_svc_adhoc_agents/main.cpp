/*
 * A simple service handler test with use ad-hoc agents.
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

struct msg_get_status : public so_5::signal_t {};

void
define_convert_service(
	so_5::coop_t & coop,
	const so_5::mbox_t & self_mbox )
	{
		coop.define_agent()
			.event( self_mbox, []( const msg_convert & msg ) -> std::string
				{
					std::ostringstream s;
					s << msg.m_value;

					return s.str();
				} )
			.event< msg_get_status >( self_mbox,
				[]() -> std::string
				{
					return "ready";
				} );
	}

struct msg_shutdown : public so_5::signal_t {};

void
define_shutdown_service(
	so_5::coop_t & coop,
	const so_5::mbox_t & self_mbox )
	{
		auto & env = coop.environment();
		coop.define_agent()
			.event< msg_shutdown >( self_mbox, [&env]() { env.stop(); } );
	}

void
compare_and_abort_if_missmatch(
	const std::string & actual,
	const std::string & expected )
	{
		if( actual != expected )
			{
				std::cerr << "VALUE MISSMATCH: actual='"
						<< actual << "', expected='" << expected << "'"
						<< std::endl;
				std::abort();
			}
	}

void
define_client(
	so_5::coop_t & coop,
	const so_5::mbox_t & svc_mbox )
	{
		coop.define_agent()
			.on_start(
				[svc_mbox]()
				{
					auto svc_proxy = svc_mbox->get_one< std::string >();

					auto c1 = svc_proxy.async( new msg_convert( 1 ) );
					auto c2 = svc_proxy.async( new msg_convert( 2 ) );

					compare_and_abort_if_missmatch(
							svc_proxy.wait_forever().sync_get( new msg_convert( 3 ) ),
							"3" );

					compare_and_abort_if_missmatch( c2.get(), "2" );
					compare_and_abort_if_missmatch( c1.get(), "1" );

					compare_and_abort_if_missmatch(
							svc_proxy.wait_forever().sync_get< msg_get_status >(),
							"ready" );

					svc_mbox->run_one().wait_forever().sync_get< msg_shutdown >();
				} );
	}

void
run_test()
	{
		so_5::launch(
			[]( so_5::environment_t & env )
			{
				auto coop = env.create_coop(
						"test_coop",
						so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

				auto svc_mbox = env.create_mbox();

				coop->add_agent( new a_time_sentinel_t( env ) );

				define_convert_service( *coop, svc_mbox );
				define_shutdown_service( *coop, svc_mbox );
				define_client( *coop, svc_mbox );

				env.register_coop( std::move( coop ) );
			},
			[]( so_5::environment_params_t & p )
			{
				p.add_named_dispatcher(
					"active_obj",
					so_5::disp::active_obj::create_disp() );
			} );
	}

int
main()
	{
		try
			{
				run_test();
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}

