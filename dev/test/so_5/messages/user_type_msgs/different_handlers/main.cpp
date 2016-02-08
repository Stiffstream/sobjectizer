/*
 * A simple test for different formats of event handlers for
 * messages of arbitrary types.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
{
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

	struct stop : public so_5::signal_t {};

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event( [&]( const int & evt ) {
					m_accumulator += "i{" + std::to_string( evt ) + "}";
				} )
			.event( [&]( const long evt ) {
					m_accumulator += "l{" + std::to_string( evt ) + "}";
				} )
			.event( &a_test_t::evt_uint )
			.event( &a_test_t::evt_ulong )
			.event( [&]( so_5::mhood_t< short > evt ) {
					m_accumulator += "si{" + std::to_string( *evt ) + "}";
				} )
			.event( &a_test_t::evt_ushort )
			.event( [&]( const std::string & evt ) {
					m_accumulator += "s{" + evt + "}";
				} )
			.event( [&]( const msg1 & evt ) {
					m_accumulator += "m1{" + evt.m_a + "," + evt.m_b + "}";
				} )
			.event( &a_test_t::evt_msg2 )
			.event< stop >( [&]{
				const std::string expected =
						"i{1}l{2}ui{3}ul{4}si{5}usi{6}s{Hello}"
						"m1{Bye,World}m2{Bye,Bye}";

				if( expected != m_accumulator )
					throw std::runtime_error( "unexpected accumulator value: " +
							m_accumulator + ", expected: " + expected );

				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send_to_agent< int >( *this, 1 );
		so_5::send_to_agent< long >( *this, 2 );
		so_5::send_to_agent< unsigned int >( *this, 3u );
		so_5::send_to_agent< unsigned long >( *this, 4ul );
		so_5::send_to_agent< short >( *this, static_cast< short >(5) );
		so_5::send_to_agent< unsigned short >( *this, static_cast< unsigned short >(6) );
		so_5::send_to_agent< std::string >( *this, "Hello" );
		so_5::send_to_agent< msg1 >( *this, "Bye", "World" );
		so_5::send_to_agent< msg2 >( *this, "Bye", "Bye" );

		so_5::send_to_agent< stop >( *this );
	}

private :
	std::string m_accumulator;

	void
	evt_uint( const unsigned int & evt )
	{
		m_accumulator += "ui{" + std::to_string( evt ) + "}";
	}

	void
	evt_ulong( unsigned long evt )
	{
		m_accumulator += "ul{" + std::to_string( evt ) + "}";
	}

	void
	evt_ushort( const so_5::mhood_t< unsigned short > & evt )
	{
		m_accumulator += "usi{" + std::to_string( *evt ) + "}";
	}

	void
	evt_msg2( msg2 evt )
	{
		m_accumulator += "m2{" + evt.m_a + "," + evt.m_b + "}";
	}
};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( so_5::autoname,
			env.make_agent< a_test_t >() );
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
			"simple user message type test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

