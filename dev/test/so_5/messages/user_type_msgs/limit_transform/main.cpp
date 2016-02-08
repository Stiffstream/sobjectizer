/*
 * A simple test for limit_then_transform for user type message.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
{
	struct transformed
	{
		std::string m_src;
		std::string m_new;
	};

	struct stop : public so_5::signal_t {};

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx
				+ limit_then_transform( 1, [this]( const std::string & v ) {
						return make_transformed< transformed >(
								so_direct_mbox(),
								v, "<" + v + ">" );
					} )
				+ limit_then_drop< stop >( 1 )
				+ limit_then_drop< transformed >( 1 )
			)
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event( [&]( const std::string & evt ) {
					m_accumulator += "s{" + evt + "}";
				} )
			.event( [&]( const transformed & evt ) {
					m_accumulator += "t{" + evt.m_src + "," + evt.m_new + "}";
				} )
			.event< stop >( [&]{
				const std::string expected = "s{One}t{Two,<Two>}";

				if( expected != m_accumulator )
					throw std::runtime_error( "unexpected accumulator value: " +
							m_accumulator + ", expected: " + expected );

				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send_to_agent< std::string >( *this, "One" );
		so_5::send_to_agent< std::string >( *this, "Two" );

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

