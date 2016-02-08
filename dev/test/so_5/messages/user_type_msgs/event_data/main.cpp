/*
 * A simple test for redirecting the same instance of message of user type.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
{
public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
		,	m_m1( ctx.environment().create_mbox() )
		,	m_m2( ctx.environment().create_mbox() )
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe( m_m1 ).event( &a_test_t::evt_one );
		so_subscribe( m_m2 ).event( &a_test_t::evt_two );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< std::string >( m_m1, "Hello, World!" );
	}

private :
	const so_5::mbox_t m_m1;
	const so_5::mbox_t m_m2;

	void
	evt_one( const so_5::mhood_t< std::string > & evt )
	{
		std::cout << "One: '" << *evt << "' at " << evt.get() << std::endl;
		m_m2->deliver_message( evt.make_reference() );
	}

	void
	evt_two( const std::string & evt )
	{
		std::cout << "Two: '" << evt << "' at " << &evt << std::endl;

		so_deregister_agent_coop_normally();
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
			"simple test for resending same instance of user type message" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

