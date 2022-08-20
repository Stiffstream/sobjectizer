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

struct default_filter_setter_t
{
	static void
	set( so_5::agent_t & to, const so_5::mbox_t & mbox )
	{
		to.so_set_delivery_filter_for_mutable_msg( mbox,
			[]( const data & msg ) {
				return 1 == msg.m_key;
			} );
	}
};

struct filter_as_variable_setter_t
{
	static void
	set( so_5::agent_t & to, const so_5::mbox_t & mbox )
	{
		auto filter = []( const data & msg ) {
				return 1 == msg.m_key;
			};
		to.so_set_delivery_filter_for_mutable_msg( mbox, filter );
	}
};

struct filter_as_const_setter_t
{
	static void
	set( so_5::agent_t & to, const so_5::mbox_t & mbox )
	{
		const auto filter = []( const data & msg ) {
				return 1 == msg.m_key;
			};
		to.so_set_delivery_filter_for_mutable_msg( mbox, filter );
	}
};

template< typename Delivery_Filter_Setter >
class a_test_t : public so_5::agent_t
{
public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		Delivery_Filter_Setter::set( *this, so_direct_mbox() );

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
	unsigned int m_values_accepted = 0;
};

void
test_case_default_filter_setter( so_5::environment_t & env )
{
	env.register_agent_as_coop(
			env.make_agent< a_test_t< default_filter_setter_t > >() );
}

void
test_case_filter_as_variable_setter( so_5::environment_t & env )
{
	env.register_agent_as_coop(
			env.make_agent< a_test_t< filter_as_variable_setter_t > >() );
}

void
test_case_filter_as_const_setter( so_5::environment_t & env )
{
	env.register_agent_as_coop(
			env.make_agent< a_test_t< filter_as_const_setter_t > >() );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &test_case_default_filter_setter );
				so_5::launch( &test_case_filter_as_variable_setter );
				so_5::launch( &test_case_filter_as_const_setter );
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

