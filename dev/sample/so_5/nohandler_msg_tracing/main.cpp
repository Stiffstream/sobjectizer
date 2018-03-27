/*
 * An example to show custom msg_tracing filter.
 * Only trace message for cases when no event handler found will be printed.
 */

#include <iostream>
#include <time.h>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Main example agent.
class a_example_t final : public so_5::agent_t
{
	// Several signals for demonstration.
	struct first final : public so_5::signal_t {};
	struct second final : public so_5::signal_t {};
	struct third final : public so_5::signal_t {};

	// Signal for finishing the example.
	struct finish final : public so_5::signal_t {};

	// Several agent's states.
	const state_t st_first{ this, "first" };
	const state_t st_second{ this, "second" };
	const state_t st_third{ this, "third" };
		
public :
	a_example_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{
		st_first.event( &a_example_t::on_first );
		st_first.event( &a_example_t::on_finish );

		st_second.event( &a_example_t::on_second );

		st_third.event( &a_example_t::on_third );
	}

	virtual void so_evt_start() override
	{
		// Change the state of the agent.
		this >>= st_first;

		// Send a serie of messages.
		so_5::send< first >( *this );
		so_5::send< second >( *this );
		so_5::send< third >( *this );

		so_5::send< finish >( *this );
	}

private :
	void on_first( mhood_t<first> ) {}
	void on_second( mhood_t<first> ) {}
	void on_third( mhood_t<first> ) {}

	void on_finish( mhood_t<finish> )
	{
		so_deregister_agent_coop_normally();
	}
};

int main()
{
	try
	{
		so_5::launch( []( so_5::environment_t & env ) {
				env.introduce_coop( []( so_5::coop_t & coop ) {
					coop.make_agent< a_example_t >();
				} );
			},
			[]( so_5::environment_params_t & params ) {
				// Turn message delivery tracing on.
				params.message_delivery_tracer(
						so_5::msg_tracing::std_cout_tracer() );
				// Setup filter which enables only messages with
				// null event_handler_data_ptr.
				params.message_delivery_tracer_filter(
						so_5::msg_tracing::make_filter(
							[](const so_5::msg_tracing::trace_data_t & td) {
								const auto h = td.event_handler_data_ptr();
								return h && (nullptr == *h);
							} ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

