/*
 * A simple test for message delivery tracing.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../simple_tracer.hpp"

class deny_msg_filter_t : public so_5::msg_tracing::filter_t
{
public :
	deny_msg_filter_t() {}

	virtual bool
	filter( const so_5::msg_tracing::trace_data_t & d ) SO_5_NOEXCEPT override
	{
		const auto ms = d.message_or_signal();
		if( ms )
			return so_5::msg_tracing::message_or_signal_flag_t::message != *ms;
		return true;
	}
};

struct finish : public so_5::signal_t {};

struct lost_signal : public so_5::signal_t {};

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
		so_subscribe( m_data_mbox ).event< finish >( &a_test_t::evt_finish );
		so_subscribe( m_data_mbox ).event( &a_test_t::evt_dummy_msg );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< dummy_msg >( m_data_mbox, 1 );
		so_5::send< finish >( m_data_mbox );
		so_5::send< lost_signal >( m_data_mbox );
	}

private :
	const so_5::mbox_t m_data_mbox;

	void
	evt_finish()
	{
		so_deregister_agent_coop_normally();
	}

	void
	evt_dummy_msg( const dummy_msg & /*msg*/ )
	{
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
				counter_t msg_counter = { 0 };
				so_5::launch( &init,
					[&]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer(
								so_5::msg_tracing::tracer_unique_ptr_t{
										new tracer_t{ msg_counter,
												so_5::msg_tracing::std_cout_tracer() } } );
						params.message_delivery_tracer_filter( new deny_msg_filter_t{} );
					} );

				const unsigned int expected_msg_count_value = 3;

				const auto actual_msg_count_value =
						msg_counter.load( std::memory_order_acquire );

				if( expected_msg_count_value != actual_msg_count_value )
					throw std::runtime_error( "Unexpected count of trace messages: "
							"expected=" +
							std::to_string(expected_msg_count_value) +
							", actual=" +
							std::to_string(actual_msg_count_value) );
			},
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

