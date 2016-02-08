/*
 * A simple test for message delivery tracing inside overlimit reaction
 * (drop_message case).
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
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx
				+ limit_then_drop< dummy_msg >(1)
				+ limit_then_abort< finish >(1) }
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self().event< finish >( &a_test_t::evt_finish );
		so_subscribe_self().event( &a_test_t::evt_dummy_msg );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< dummy_msg >( *this, 1 );
		so_5::send< dummy_msg >( *this, 2 );
	}

private :
	void
	evt_finish()
	{
		so_deregister_agent_coop_normally();
	}

	void
	evt_dummy_msg( const dummy_msg & msg )
	{
		std::cout << "message received: " << msg.m_i << std::endl;
		so_5::send< finish >( *this );
	}
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop( []( so_5::coop_t & coop ) {
			coop.make_agent< a_test_t >();
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

				const unsigned int expected_value = 5;
				auto actual_value = counter.load( std::memory_order_acquire );
				if( expected_value != actual_value )
					throw std::runtime_error( "Unexpected count of trace messages: "
							"expected=" + std::to_string(expected_value) +
							", actual=" + std::to_string(actual_value) );
			},
			20,
			"simple tracing overlimit reaction (drop_message case)" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

