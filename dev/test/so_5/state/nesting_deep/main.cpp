/*
 * A very simple test case for checking deep of agent state nesting.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t final : public so_5::agent_t
{
	struct sig_1 : public so_5::signal_t {};

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		m_states.push_back( state_unique_ptr{ new state_t{ this, "s" } } );
		for( std::size_t i = 1; i < state_t::max_deep; ++i )
		{
			state_unique_ptr s{ new state_t{
					initial_substate_of{ *m_states.back() }, "s" } };
			m_states.push_back( std::move(s) );
		}

		try
		{
			state_unique_ptr s{ new state_t{
					initial_substate_of{ *m_states.back() } } };
			m_states.push_back( std::move(s) );

			throw std::runtime_error( "exception must be throw on attempt "
					"to create another nested state!" );
		}
		catch( const so_5::exception_t & ex )
		{
			std::cout << "Exception: " << ex.what() << std::endl;
			if( so_5::rc_state_nesting_is_too_deep != ex.error_code() )
				throw std::runtime_error( "expected error_code: " +
						std::to_string( ex.error_code() ) );
		}

		m_states.front()->event< sig_1 >( [this] {
				so_deregister_agent_coop_normally();
			} );

		this >>= *m_states.front();
	}

	virtual void
	so_evt_start() override
	{
		std::cout << so_current_state().query_name() << std::endl;

		so_5::send< sig_1 >( *this );
	}

private :
	using state_unique_ptr = std::unique_ptr< state_t >;

	std::vector< state_unique_ptr > m_states;
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( []( so_5::environment_t & env ) {
						env.introduce_coop( []( so_5::coop_t & coop ) {
								coop.make_agent< a_test_t >();
							} );
					}/*,
					[]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					}*/ );
			},
			20,
			"simple test for too deep nesting of agent states" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

