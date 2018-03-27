/*
 * A simple check for change_message_delivery_tracer_filter() method.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../simple_tracer.hpp"

class a_test_t : public so_5::agent_t
{
	struct next_step : public so_5::signal_t {};
public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
		,	m_mbox1( so_environment().create_mbox() )
		,	m_mbox2( so_environment().create_mbox() )
		,	m_mbox3( so_environment().create_mbox() )
	{}

	virtual void
	so_evt_start() override
	{
		so_subscribe( m_mbox1 ).event( &a_test_t::first_event );

		send_messages();
	}

private :
	const so_5::mbox_t m_mbox1;
	const so_5::mbox_t m_mbox2;
	const so_5::mbox_t m_mbox3;

	void
	send_messages()
	{
		so_5::send< next_step >( m_mbox1 );
		so_5::send< next_step >( m_mbox2 );
		so_5::send< next_step >( m_mbox3 );
	}

	void
	first_event( mhood_t<next_step> )
	{
		so_drop_subscription( m_mbox1, &a_test_t::first_event );
		so_subscribe( m_mbox2 ).event( &a_test_t::second_event );

		so_environment().change_message_delivery_tracer_filter(
			make_filter( m_mbox2 ) );

		send_messages();
	}

	void
	second_event( mhood_t<next_step> )
	{
		so_drop_subscription( m_mbox2, &a_test_t::second_event );
		so_subscribe( m_mbox3 ).event( &a_test_t::third_event );

		so_environment().change_message_delivery_tracer_filter(
			so_5::msg_tracing::make_enable_all_filter() );

		send_messages();
	}

	void
	third_event( mhood_t<next_step> )
	{
		so_environment().change_message_delivery_tracer_filter(
			so_5::msg_tracing::make_disable_all_filter() );

		so_deregister_agent_coop_normally();
	}

	so_5::msg_tracing::filter_shptr_t
	make_filter( const so_5::mbox_t & mbox ) {
		const auto id = mbox->id();
		return so_5::msg_tracing::make_filter(
				[id]( const so_5::msg_tracing::trace_data_t & td ) {
					const auto ms = td.msg_source();
					return ms && id == ms->m_id;
				} );
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

						params.message_delivery_tracer_filter(
								so_5::msg_tracing::make_disable_all_filter() );
					} );

				const unsigned int expected_value = 6;
				auto actual_value = counter.load( std::memory_order_acquire );
				if( expected_value != actual_value )
					throw std::runtime_error( "Unexpected count of trace messages: "
							"expected=" + std::to_string(expected_value) +
							", actual=" + std::to_string(actual_value) );
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

