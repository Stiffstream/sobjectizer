/*
 * A simple test for delivery filters for messages of user types.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
{
	struct msg
	{
		std::string m_a;
		std::string m_b;
	};

	struct stop : public so_5::signal_t {};

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
		,	m_mbox( ctx.env().create_mbox() )
	{}

	virtual void
	so_define_agent() override
	{
		so_set_delivery_filter( m_mbox,
			[]( int v ) { return v > 3; } );
		so_set_delivery_filter( m_mbox,
			[]( const std::string & v ) { return v.length() == 5; } );
		so_set_delivery_filter( m_mbox,
			[]( const msg & v ) { return "Bye" == v.m_a; } );

		so_subscribe( m_mbox )
			.event( [&]( int evt ) {
					m_accumulator += "i{" + std::to_string( evt ) + "}";
				} )
			.event( [&]( const std::string & evt ) {
					m_accumulator += "s{" + evt + "}";
				} )
			.event( [&]( const msg & evt ) {
					m_accumulator += "m{" + evt.m_a + "," + evt.m_b + "}";
				} )
			.event< stop >( [&]{
				const std::string expected = "i{4}s{Hello}m{Bye,World}";

				if( expected != m_accumulator )
					throw std::runtime_error( "unexpected accumulator value: " +
							m_accumulator + ", expected: " + expected );

				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< int >( m_mbox, 1 );
		so_5::send< int >( m_mbox, 3 );
		so_5::send< int >( m_mbox, 4 );

		so_5::send< std::string >( m_mbox, "Bye" );
		so_5::send< std::string >( m_mbox, "Hello" );
		so_5::send< std::string >( m_mbox, "Hello, World!" );

		so_5::send< msg >( m_mbox, "Hello", "World" );
		so_5::send< msg >( m_mbox, "Bye", "World" );
		so_5::send< msg >( m_mbox, "Bye-Bye", "World" );

		so_5::send< stop >( m_mbox );
	}

private :
	const so_5::mbox_t m_mbox;

	std::string m_accumulator;
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
			"simple delivery filter for user message type test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

