/*
 * A simple test for message delivery tracing on limitful MPSC mbox.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../simple_tracer.hpp"

struct start : public so_5::signal_t {};
struct finish : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
public :
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx
				+ limit_then_abort< start >( 1 )
				+ limit_then_abort< finish >( 1 ) }
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self().event< start >( &a_test_t::evt_start );
		so_subscribe_self().event< finish >( &a_test_t::evt_finish );
	}

private :
	void
	evt_start()
	{
		so_5::send< finish >( *this );
	}

	void
	evt_finish()
	{
		so_deregister_agent_coop_normally();
	}
};

class a_request_initator_t : public so_5::agent_t
{
public :
	a_request_initator_t( context_t ctx, so_5::mbox_t other_mbox )
		:	so_5::agent_t{ ctx }
		,	m_other_mbox{ std::move( other_mbox ) }
		{}

	virtual void
	so_evt_start() override
	{
		so_5::request_value< void, start >( m_other_mbox, so_5::infinite_wait );
	}

private :
	const so_5::mbox_t m_other_mbox;
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop(
		so_5::disp::active_obj::create_private_disp( env )->binder(),
		[]( so_5::coop_t & coop ) {
			auto a_test = coop.make_agent< a_test_t >();
			coop.make_agent< a_request_initator_t >( a_test->so_direct_mbox() );
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

				const unsigned int expected_value = 4;
				auto actual_value = counter.load( std::memory_order_acquire );
				if( expected_value != actual_value )
					throw std::runtime_error( "Unexpected count of trace messages: "
							"expected=" + std::to_string(expected_value) +
							", actual=" + std::to_string(actual_value) );
			},
			20,
			"simple tracing for limitless MPSC-mboxes" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

