/*
 * A simple test for delivery filters for mutable messages and MPSC mboxes.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct data { int m_key; };

struct finish : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		so_set_delivery_filter_for_mutable_msg( so_direct_mbox(),
			[]( const data & msg ) {
				return 1 == msg.m_key;
			} );

		so_default_state()
			.event( so_direct_mbox(), [this]( mutable_mhood_t< data > cmd ) {
				const auto value = cmd->m_key;
				if( 1 != value )
					throw std::runtime_error( "unexpected data value: " +
							std::to_string( value ) );
				++m_values_accepted;
			} )
			.event( [this](mhood_t< finish >) {
				if( 2 != m_values_accepted )
					throw std::runtime_error( "unexpected count of "
							"accepted data instances: " +
							std::to_string( m_values_accepted ) );

				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< so_5::mutable_msg<data> >( so_direct_mbox(), 0 );
		so_5::send< so_5::mutable_msg<data> >( so_direct_mbox(), 1 );
		so_5::send< so_5::mutable_msg<data> >( so_direct_mbox(), 2 );
		so_5::send< so_5::mutable_msg<data> >( so_direct_mbox(), 3 );
		so_5::send< so_5::mutable_msg<data> >( so_direct_mbox(), 4 );
		so_5::send< so_5::mutable_msg<data> >( so_direct_mbox(), 0 );
		so_5::send< so_5::mutable_msg<data> >( so_direct_mbox(), 1 );
		so_5::send< so_5::mutable_msg<data> >( so_direct_mbox(), 2 );
		so_5::send< so_5::mutable_msg<data> >( so_direct_mbox(), 3 );
		so_5::send< so_5::mutable_msg<data> >( so_direct_mbox(), 4 );

		so_5::send< finish >( *this );
	}

private :
	const so_5::mbox_t m_data_mbox;
	unsigned int m_values_accepted = 0;
};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( env.make_agent< a_test_t >() );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch(
					&init/*,
					[]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
					}*/ );
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

