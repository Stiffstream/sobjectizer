/*
 * A simple test for service requests of user types.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg1
{
	std::string m_a;
	std::string m_b;
};

struct msg2
{
	std::string m_a;
	std::string m_b;
};

class a_service_t : public so_5::agent_t
{
public :
	a_service_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event( [&]( const int & evt ) -> std::string {
					return "i{" + std::to_string( evt ) + "}";
				} )
			.event( [&]( long evt ) -> std::string {
					return "l{" + std::to_string( evt ) + "}";
				} )
			.event( &a_service_t::evt_uint )
			.event( &a_service_t::evt_ulong )
			.event( [&]( const std::string & evt ) {
					return "s{" + evt + "}";
				} )
			.event( [&]( const msg1 & evt ) {
					return "m1{" + evt.m_a + "," + evt.m_b + "}";
				} )
			.event( &a_service_t::evt_msg2 );
	}

private :
	std::string
	evt_uint( const unsigned int & evt )
	{
		return "ui{" + std::to_string( evt ) + "}";
	}

	std::string
	evt_ulong( unsigned long evt )
	{
		return "ul{" + std::to_string( evt ) + "}";
	}

	std::string
	evt_msg2( msg2 evt )
	{
		return "m2{" + evt.m_a + "," + evt.m_b + "}";
	}
};

class a_test_t : public so_5::agent_t
{
public :
	a_test_t( context_t ctx, so_5::mbox_t service )
		:	so_5::agent_t( ctx )
		,	m_service( std::move( service ) )
	{}

	virtual void
	so_evt_start() override
	{
		auto svc = m_service->get_one< std::string >().wait_forever();

		std::string accumulator;

		accumulator += svc.make_sync_get< int >( 1 );
		accumulator += svc.make_sync_get< long >( 2 );
		accumulator += svc.make_sync_get< unsigned int >( 3u );
		accumulator += svc.make_sync_get< unsigned long >( 4ul );
		accumulator += svc.make_sync_get< std::string >( "Hello" );
		accumulator += svc.make_sync_get< msg1 >( "Bye", "World" );
		accumulator += svc.make_sync_get< msg2 >( "Bye", "Bye" );

		const std::string expected =
				"i{1}l{2}ui{3}ul{4}s{Hello}"
				"m1{Bye,World}m2{Bye,Bye}";

		if( expected != accumulator )
			throw std::runtime_error( "unexpected accumulator value: " +
					accumulator + ", expected: " + expected );

		so_deregister_agent_coop_normally();
	}

private :
	const so_5::mbox_t m_service;
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop( []( so_5::coop_t & coop ) {
			using namespace so_5::disp::one_thread;

			auto service = coop.make_agent_with_binder< a_service_t >(
					create_private_disp( coop.environment() )->binder() );

			coop.make_agent< a_test_t >( service->so_direct_mbox() );
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			20,
			"simple user message type service_request test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

