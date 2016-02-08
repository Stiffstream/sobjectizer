/*
 * A simple test for v.5.5.9 helper functions for synchronous
 * interactions.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct classic_msg : public so_5::message_t
{
	std::string m_a;
	std::string m_b;

	classic_msg( std::string a, std::string b )
		:	m_a( std::move(a) ), m_b( std::move(b) )
		{}
};

struct msg
{
	std::string m_a;
	std::string m_b;
};

struct empty {};

struct classic_signal : public so_5::signal_t {};

template< typename TARGET, typename MBOX >
void
setup_service_events( TARGET & to, MBOX mbox )
{
	to
		.event( mbox, []( int evt ) -> std::string {
				return "i{" + std::to_string( evt ) + "}";
			} )
		.event( mbox, []( const classic_msg & evt ) {
				return "cm{" + evt.m_a + "," + evt.m_b + "}";
			} )
		.event( mbox, []( const msg & evt ) -> std::string {
				return "m{" + evt.m_a + "," + evt.m_b + "}";
			} )
		.event( mbox, []( empty ) -> std::string {
				return "empty{}";
			} )
		.template event< classic_signal >( mbox, []() -> std::string {
				return "signal{}";
			} );
}

template< typename TARGET >
void
perform_service_interaction_via_futures( TARGET && service )
{
	std::string accumulator;

	accumulator += so_5::request_future< std::string, int >( service, 1 ).get();

	accumulator += so_5::request_future< std::string, classic_msg >(
			service, "Hello", "World" ).get();

	accumulator += so_5::request_future< std::string, msg >(
			service, "Bye", "World" ).get();

	accumulator += so_5::request_future< std::string, classic_signal >(
			service ).get();

	accumulator += so_5::request_future< std::string, empty >(
			service ).get();

	const std::string expected = "i{1}cm{Hello,World}m{Bye,World}"
			"signal{}empty{}";

	if( expected != accumulator )
		throw std::runtime_error( "unexpected accumulator value: " +
				accumulator + ", expected: " + expected );
}

template< typename TARGET >
void
perform_service_interaction_via_infinite_wait( TARGET && service )
{
	using namespace so_5;

	std::string accumulator;

	accumulator += request_value< std::string, int >(
			service, infinite_wait, 1 );

	accumulator += request_value< std::string, classic_msg >(
			service, infinite_wait, "Hello", "World" );

	accumulator += request_value< std::string, msg >(
			service, infinite_wait, "Bye", "World" );

	accumulator += request_value< std::string, classic_signal >(
			service, infinite_wait );

	accumulator += request_value< std::string, empty >(
			service, infinite_wait );

	const std::string expected = "i{1}cm{Hello,World}m{Bye,World}"
			"signal{}empty{}";

	if( expected != accumulator )
		throw std::runtime_error( "unexpected accumulator value: " +
				accumulator + ", expected: " + expected );
}

template< typename TARGET >
void
perform_service_interaction_via_finite_wait( TARGET && service )
{
	using namespace so_5;
	using secs = std::chrono::seconds;

	std::string accumulator;

	accumulator += request_value< std::string, int >(
			service, secs(5), 1 );

	accumulator += request_value< std::string, classic_msg >(
			service, secs(5), "Hello", "World" );

	accumulator += request_value< std::string, msg >(
			service, secs(5), "Bye", "World" );

	accumulator += request_value< std::string, classic_signal >(
			service, secs(5) );

	accumulator += request_value< std::string, empty >(
			service, secs(5) );

	const std::string expected = "i{1}cm{Hello,World}m{Bye,World}"
			"signal{}empty{}";

	if( expected != accumulator )
		throw std::runtime_error( "unexpected accumulator value: " +
				accumulator + ", expected: " + expected );
}

template< typename TARGET >
void
perform_service_interaction( TARGET && service )
{
	using namespace std;

	perform_service_interaction_via_futures( forward< TARGET >(service) );
	perform_service_interaction_via_infinite_wait( forward< TARGET >(service) );
	perform_service_interaction_via_finite_wait( forward< TARGET >(service) );
}

class a_service_t : public so_5::agent_t
{
public :
	a_service_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		setup_service_events( so_default_state(), so_direct_mbox() );
	}
};

class a_test_via_mbox_t : public so_5::agent_t
{
public :
	a_test_via_mbox_t( context_t ctx, so_5::mbox_t service )
		:	so_5::agent_t( ctx )
		,	m_service( std::move( service ) )
	{}

	virtual void
	so_evt_start() override
	{
		std::string accumulator;

		perform_service_interaction( m_service );

		so_deregister_agent_coop_normally();
	}

private :
	const so_5::mbox_t m_service;
};

class a_test_via_direct_mbox_t : public so_5::agent_t
{
public :
	a_test_via_direct_mbox_t( context_t ctx, const a_service_t & service )
		:	so_5::agent_t( ctx )
		,	m_service( service )
	{}

	virtual void
	so_evt_start() override
	{
		std::string accumulator;

		perform_service_interaction( m_service );

		so_deregister_agent_coop_normally();
	}

private :
	const a_service_t & m_service;
};

void
make_adhoc_agents_coop( so_5::environment_t & env )
{
	env.introduce_coop( []( so_5::coop_t & coop ) {
		using namespace so_5::disp::one_thread;

		auto service = coop.define_agent(
				create_private_disp( coop.environment() )->binder() );
		setup_service_events( service, service );

		coop.define_agent().on_start( [&coop, service] {
				perform_service_interaction( service );

				coop.deregister_normally();
			} );
	} );
}

void
init( so_5::environment_t & env )
{
	env.introduce_coop( []( so_5::coop_t & coop ) {
			using namespace so_5::disp::one_thread;

			auto service = coop.make_agent_with_binder< a_service_t >(
					create_private_disp( coop.environment() )->binder() );

			coop.make_agent< a_test_via_mbox_t >( service->so_direct_mbox() );
		} );

	env.introduce_coop( []( so_5::coop_t & coop ) {
			using namespace so_5::disp::one_thread;

			auto service = coop.make_agent_with_binder< a_service_t >(
					create_private_disp( coop.environment() )->binder() );

			coop.make_agent< a_test_via_direct_mbox_t >( *service );
		} );

	make_adhoc_agents_coop( env );
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

