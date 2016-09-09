/*
 * A test for defining only delivery filter without subscriptions.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct data { int m_key; };

struct one : public so_5::signal_t {};
struct finish : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
		,	m_data_mbox( so_environment().create_mbox() )
	{}

	virtual void
	so_define_agent() override
	{
		so_set_delivery_filter( m_data_mbox,
			[]( const data & msg ) {
				return 1 == msg.m_key;
			} );

		so_default_state()
			.event< one >( [this] {
				so_drop_delivery_filter< data >( m_data_mbox );

				so_5::send< data >( m_data_mbox, 0 );
				so_5::send< data >( m_data_mbox, 1 );
				so_5::send< data >( m_data_mbox, 2 );

				so_5::send_to_agent< finish >( *this );
			} )
			.event< finish >( [this] {
				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< data >( m_data_mbox, 0 );
		so_5::send< data >( m_data_mbox, 1 );
		so_5::send< data >( m_data_mbox, 2 );

		so_5::send_to_agent< one >( *this );
	}

private :
	const so_5::mbox_t m_data_mbox;
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
			"delivery filter without subscriptions test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

