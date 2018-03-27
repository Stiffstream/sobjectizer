/*
 * A simple test for message delivery tracing inside overlimit reaction
 * (redirect case).
 *
 * With message delivery tracing with filter for overlimit messages.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../simple_tracer.hpp"

struct hello : public so_5::signal_t {};

class a_first_t : public so_5::agent_t
{
public :
	a_first_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self().event< hello >( &a_first_t::evt_hello );
	}

private :
	void
	evt_hello()
	{
		so_deregister_agent_coop_normally();
	}
};

class a_second_t : public so_5::agent_t
{
public :
	a_second_t( context_t ctx, so_5::mbox_t target )
		:	so_5::agent_t{ ctx
				+ limit_then_redirect< hello >( 1,
						[this]{ return m_target; } ) }
		,	m_target{ std::move(target) }
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self().event< hello >( &a_second_t::evt_hello );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< hello >( *this );
		so_5::send< hello >( *this );
	}

private :
	const so_5::mbox_t m_target;

	void
	evt_hello()
	{
	}
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop( []( so_5::coop_t & coop ) {
			auto first = coop.make_agent< a_first_t >();
			coop.make_agent< a_second_t >( first->so_direct_mbox() );
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
								so_5::msg_tracing::make_filter(
										[]( const so_5::msg_tracing::trace_data_t & d ) {
											const auto ca = d.compound_action();
											if( ca ) {
												return nullptr != std::strstr(
														ca->m_second, "overlimit" );
											}

											return false;
										} ) );
					} );

				const unsigned int expected_value = 1;
				auto actual_value = counter.load( std::memory_order_acquire );
				if( expected_value != actual_value )
					throw std::runtime_error( "Unexpected count of trace messages: "
							"expected=" + std::to_string(expected_value) +
							", actual=" + std::to_string(actual_value) );
			},
			20,
			"simple tracing overlimit reaction (redirect case)" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

