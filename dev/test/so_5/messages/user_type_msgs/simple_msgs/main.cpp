/*
 * A simple test for messages of user types.
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
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event( [&]( const int & evt ) {
					m_accumulator += "i{" + std::to_string( evt ) + "}";
				} )
			.event( [&]( const std::string & evt ) {
					m_accumulator += "s{" + evt + "}";
				} )
			.event( [&]( const msg & evt ) {
					m_accumulator += "m{" + evt.m_a + "," + evt.m_b + "}";
				} )
			.event< stop >( [&]{
				if( "i{1}s{Hello}m{Bye,World}" != m_accumulator )
					throw std::runtime_error( "unexpected accumulator value: " +
							m_accumulator );

				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send_to_agent< int >( *this, 1 );
		so_5::send_to_agent< std::string >( *this, "Hello" );
		so_5::send_to_agent< msg >( *this, "Bye", "World" );

		so_5::send_to_agent< stop >( *this );
	}

private :
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
			"simple user message type test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

