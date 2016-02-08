/*
 * A simple test for message delivery tracing in the case of service requests
 * and an exception during delivery.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../simple_tracer.hpp"

struct finish : public so_5::signal_t {};

class a_request_initator_t : public so_5::agent_t
{
public :
	a_request_initator_t( context_t ctx, so_5::mbox_t data_mbox )
		:	so_5::agent_t{ ctx }
		,	m_data_mbox{ std::move( data_mbox ) }
		{}

	virtual void
	so_evt_start() override
	{
		try
		{
			so_5::request_value< void, finish >( m_data_mbox, so_5::infinite_wait );
		}
		catch( const std::exception & x )
		{
			std::cout << "Expected exception: " << x.what() << std::endl;
			so_deregister_agent_coop_normally();
		}
	}

private :
	const so_5::mbox_t m_data_mbox;
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop(
		so_5::disp::active_obj::create_private_disp( env )->binder(),
		[]( so_5::coop_t & coop ) {
			coop.make_agent< a_request_initator_t >(
					coop.environment().create_mbox( "gate" ) );
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

				const unsigned int expected_value = 1;
				auto actual_value = counter.load( std::memory_order_acquire );
				if( expected_value != actual_value )
					throw std::runtime_error( "Unexpected count of trace messages: "
							"expected=" + std::to_string(expected_value) +
							", actual=" + std::to_string(actual_value) );
			},
			20,
			"simple tracing for service request via MPMC-mboxes" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

