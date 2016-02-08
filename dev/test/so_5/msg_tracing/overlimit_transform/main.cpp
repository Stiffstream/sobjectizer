/*
 * A simple test for message delivery tracing inside overlimit reaction
 * (drop_message case).
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../simple_tracer.hpp"

struct hello { int m_v; };
struct bye { std::string m_v; };

class a_first_t : public so_5::agent_t
{
public :
	a_first_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_first_t::evt_bye );
	}

private :
	void
	evt_bye( const bye & msg )
	{
		if( "1" == msg.m_v )
			so_deregister_agent_coop_normally();
	}
};

class a_second_t : public so_5::agent_t
{
public :
	a_second_t( context_t ctx, so_5::mbox_t target )
		:	so_5::agent_t{ ctx
				+ limit_then_transform( 1,
						[this]( const hello & msg ) {
							return make_transformed< bye >(
								m_target,
								std::to_string( msg.m_v ) );
						} ) }
		,	m_target{ std::move(target) }
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_second_t::evt_hello );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< hello >( *this, 0 );
		so_5::send< hello >( *this, 1 );
	}

private :
	const so_5::mbox_t m_target;

	void
	evt_hello( const hello & ) {}
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
					} );

				const unsigned int expected_value = 5;
				auto actual_value = counter.load( std::memory_order_acquire );
				if( expected_value != actual_value )
					throw std::runtime_error( "Unexpected count of trace messages: "
							"expected=" + std::to_string(expected_value) +
							", actual=" + std::to_string(actual_value) );
			},
			20,
			"simple tracing overlimit reaction (transform case)" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

