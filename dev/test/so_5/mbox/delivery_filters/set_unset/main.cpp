/*
 * A test for setting and unsetting filters.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct data { int m_key; };

struct next : public so_5::signal_t {};
struct finish : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
		,	m_data_mbox( so_environment().create_mbox() )
	{}

	virtual void
	so_define_agent() override
	{
		this >>= st_1;

		so_set_delivery_filter( m_data_mbox,
			[]( const data & msg ) {
				return 1 == msg.m_key;
			} );

		so_subscribe( m_data_mbox )
			.in( st_1 )
			.in( st_2 )
			.in( st_3 )
			.in( st_4 )
			.in( st_5 )
			.in( st_6 )
			.in( st_7 )
			.event( &a_test_t::evt_data );

		st_1.event< next >( [this] {
				this >>= st_2;
				m_accumulator += '|';
				so_drop_delivery_filter< data >( m_data_mbox );

				send_bunch();
			} );

		st_2.event< next >( [this] {
				this >>= st_3;
				m_accumulator += '|';
				so_set_delivery_filter( m_data_mbox, []( const data & msg ) {
					return 2 == msg.m_key;
				} );

				send_bunch();
			} );

		st_3.event< next >( [this] {
				this >>= st_4;
				m_accumulator += '|';
				so_drop_delivery_filter< data >( m_data_mbox );

				send_bunch();
			} );

		st_4.event< next >( [this] {
				this >>= st_5;
				m_accumulator += '|';
				so_set_delivery_filter( m_data_mbox, []( const data & msg ) {
					return 3 == msg.m_key;
				} );

				send_bunch();
			} );

		st_5.event< next >( [this] {
				this >>= st_6;
				m_accumulator += '|';
				so_drop_delivery_filter< data >( m_data_mbox );

				send_bunch();
			} );

		st_6.event< next >( [this] {
				this >>= st_7;
				m_accumulator += '|';
				so_set_delivery_filter( m_data_mbox, []( const data & msg ) {
					return 4 == msg.m_key;
				} );

				send_bunch();
			} );

		st_7.event< next >( [this] {
				const std::string expected =
					"1,|0,1,2,3,4,|2,|0,1,2,3,4,|3,|0,1,2,3,4,|4,";

				if( expected == m_accumulator )
					so_deregister_agent_coop_normally();
				else
					throw std::runtime_error( "values mismatch, actual: '" +
							m_accumulator + "', expected: '" + expected + "'" );
			} );
	}

	virtual void
	so_evt_start() override
	{
		send_bunch();
	}

private :
	const so_5::state_t st_1{ this };
	const so_5::state_t st_2{ this };
	const so_5::state_t st_3{ this };
	const so_5::state_t st_4{ this };
	const so_5::state_t st_5{ this };
	const so_5::state_t st_6{ this };
	const so_5::state_t st_7{ this };

	const so_5::mbox_t m_data_mbox;

	std::string m_accumulator;

	void
	send_bunch()
	{
		so_5::send< data >( m_data_mbox, 0 );
		so_5::send< data >( m_data_mbox, 1 );
		so_5::send< data >( m_data_mbox, 2 );
		so_5::send< data >( m_data_mbox, 3 );
		so_5::send< data >( m_data_mbox, 4 );

		so_5::send_to_agent< next >( *this );
	}

	void
	evt_data( const data & msg )
	{
		m_accumulator += std::to_string( msg.m_key );
		m_accumulator += ',';
	}
};

void
init( so_5::environment_t & env )
{
	auto disp = so_5::disp::thread_pool::create_private_disp( env );
	const auto params = so_5::disp::thread_pool::bind_params_t{}.
			max_demands_at_once( 1 );

	for( unsigned int i = 0; i != 1000u; ++i )
		env.register_agent_as_coop( so_5::autoname,
				env.make_agent< a_test_t >(),
				disp->binder( params ) );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			40,
			"delivery filter set/unset test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

