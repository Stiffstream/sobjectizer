/*
 * A test for removing delivery filter during subscriber deregistration.
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

struct finish : public so_5::signal_t {};

class a_child_t : public so_5::agent_t
{
public :
	a_child_t( context_t ctx, so_5::mbox_t data_mbox )
		:	so_5::agent_t( ctx )
		,	m_data_mbox( data_mbox )
	{}

	virtual void
	so_define_agent() override
	{
		so_set_delivery_filter( m_data_mbox, []( const data & msg ) {
				return 1 == msg.m_key;
			} );

		so_subscribe( m_data_mbox ).event( [this]( const data & msg ) {
				const auto value = msg.m_key;
				if( 1 == value )
					so_deregister_agent_coop_normally();
				else
					throw std::runtime_error( "unexpected value: " +
							std::to_string( value ) );
			} );
	}

private :
	const so_5::mbox_t m_data_mbox;
};

class a_parent_t : public so_5::agent_t
{
public :
	a_parent_t( context_t ctx )
		:	so_5::agent_t( ctx )
		,	m_data_mbox( so_environment().create_mbox() )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event( [this]( const so_5::msg_coop_deregistered & ) {
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
		so_5::introduce_child_coop( *this,
			[this]( so_5::coop_t & coop ) {
				coop.add_dereg_notificator(
						so_5::make_coop_dereg_notificator( so_direct_mbox() ) );
				coop.make_agent< a_child_t >( m_data_mbox );
			} );

		so_5::send< data >( m_data_mbox, 0 );
		so_5::send< data >( m_data_mbox, 1 );
		so_5::send< data >( m_data_mbox, 2 );
	}

private :
	const so_5::mbox_t m_data_mbox;
};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( so_5::autoname,
			env.make_agent< a_parent_t >() );
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
			"delivery filter for MPSC-mboxes" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

