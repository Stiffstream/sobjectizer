/*
 * A simple test for message delivery tracing.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../simple_tracer.hpp"

struct finish : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	struct dummy_msg { int m_i; };

public :
	a_test_t( context_t ctx, so_5::mbox_t data_mbox )
		:	so_5::agent_t{ ctx }
		,	m_data_mbox{ std::move( data_mbox ) }
	{}

	virtual void
	so_define_agent() override
	{
		so_set_delivery_filter( m_data_mbox, []( const dummy_msg & msg ) {
				return 0 == msg.m_i;
			} );

		so_subscribe( m_data_mbox ).event< finish >( &a_test_t::evt_finish );
		so_subscribe( m_data_mbox ).event( &a_test_t::evt_dummy_msg );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< dummy_msg >( m_data_mbox, 1 );
		so_5::send< finish >( m_data_mbox );
	}

private :
	const so_5::mbox_t m_data_mbox;

	void
	evt_finish()
	{
		so_deregister_agent_coop_normally();
	}

	void
	evt_dummy_msg( const dummy_msg & msg )
	{
		if( 0 != msg.m_i )
			throw std::runtime_error( "msg.m_i != 0" );
	}
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop( []( so_5::coop_t & coop ) {
			coop.make_agent< a_test_t >( coop.environment().create_mbox() );
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
				counter_t counter = { 0 };
				so_5::launch( &init,
					[&counter]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer(
								so_5::msg_tracing::tracer_unique_ptr_t{
										new tracer_t{ counter,
												so_5::msg_tracing::std_cout_tracer() } } );
					} );

				const unsigned int expected_value = 3;
				auto actual_value = counter.load( std::memory_order_acquire );
				if( expected_value != actual_value )
					throw std::runtime_error( "Unexpected count of trace messages: "
							"expected=" + std::to_string(expected_value) +
							", actual=" + std::to_string(actual_value) );
			},
			20,
			"simple tracing for MPMC-mboxes" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

