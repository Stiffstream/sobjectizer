/*
 * A sample for use service handlers for parallel sum of vector.
 */

#include <iostream>
#include <exception>
#include <numeric>
#include <vector>
#include <cstdlib>

#include <so_5/all.hpp>

typedef std::vector< int > vector_t;

struct msg_sum_part : public so_5::message_t
	{
		vector_t::const_iterator m_begin;
		vector_t::const_iterator m_end;

		msg_sum_part(
			const vector_t::const_iterator & b,
			const vector_t::const_iterator & e )
			:	m_begin( b )
			,	m_end( e )
			{}
	};

struct msg_sum_vector : public so_5::message_t
	{
		const vector_t & m_vector;

		msg_sum_vector( const vector_t & v ) : m_vector( v )
			{}
	};

class a_vector_summator_t : public so_5::agent_t
	{
	public :
		a_vector_summator_t( context_t ctx, so_5::mbox_t self_mbox )
			:	so_5::agent_t( ctx )
			,	m_self_mbox( std::move(self_mbox) )
			,	m_part_summator_mbox( so_environment().create_mbox() )
			{}

		virtual void so_define_agent() override
			{
				so_subscribe( m_self_mbox ).event( &a_vector_summator_t::evt_sum );
			}

		virtual void so_evt_start() override
			{
				// Create a helper agent which will work in child cooperation.
				so_5::introduce_child_coop(
						*this,
						so_5::disp::active_obj::create_disp_binder( "active_obj" ),
						[&]( so_5::coop_t & coop )
						{
							coop.define_agent().event(
								m_part_summator_mbox,
								[]( const msg_sum_part & part ) {
									return std::accumulate( part.m_begin, part.m_end, 0 );
								} );
						} );
			}

		int evt_sum( const msg_sum_vector & evt )
			{
				auto m = evt.m_vector.begin() + evt.m_vector.size() / 2;

				return std::accumulate( evt.m_vector.begin(), m, 0 ) +
						so_5::request_value< int, msg_sum_part >(
								m_part_summator_mbox, so_5::infinite_wait,
								m, evt.m_vector.end() );
			}

	private :
		const so_5::mbox_t m_self_mbox;
		const so_5::mbox_t m_part_summator_mbox;
	};

class progress_indicator_t
	{
	public :
		progress_indicator_t( std::size_t total )
			:	m_total( total )
			,	m_percents( 0 )
			{}

		~progress_indicator_t()
			{
				std::cout << std::endl;
			}

		void update( std::size_t current )
			{
				int p = static_cast< int >(
						(double(current + 1) / double(m_total)) * 100);
				if( p != m_percents )
					{
						m_percents = p;
						std::cout << m_percents << "\r" << std::flush;
					}
			}

	private :
		const std::size_t m_total;
		int m_percents;
	};

class a_runner_t : public so_5::agent_t
	{
	public :
		a_runner_t( context_t ctx, std::size_t iterations )
			:	so_5::agent_t( ctx )
			,	ITERATIONS( iterations )
			,	m_summator_mbox( so_environment().create_mbox() )
			{}

		virtual void so_evt_start() override
			{
				create_summator_coop();
				fill_test_vector();

				do_calculations();

				so_environment().stop();
			}

	private :
		const std::size_t ITERATIONS;

		const so_5::mbox_t m_summator_mbox;

		vector_t m_vector;

		void create_summator_coop()
			{
				so_5::introduce_child_coop(
					*this,
					so_5::disp::active_obj::create_disp_binder( "active_obj" ),
					[this]( so_5::coop_t & coop ) {
						coop.make_agent< a_vector_summator_t >( m_summator_mbox );
					} );
			}

		void fill_test_vector()
			{
				const std::size_t CAPACITY = 1000;
				m_vector.reserve( CAPACITY );

				for( int i = 0; i != static_cast< int >(CAPACITY); ++i )
					m_vector.push_back( i );
			}

		void do_calculations()
			{
				progress_indicator_t indicator( (ITERATIONS) );

				for( std::size_t i = 0; i != ITERATIONS; ++i )
					{
						so_5::request_value< int, msg_sum_vector >(
								m_summator_mbox, so_5::infinite_wait, m_vector );

						indicator.update( i );
					}
			}
	};

int main( int argc, char ** argv )
	{
		try
			{
				so_5::launch(
						[argc, argv]( so_5::environment_t & env ) {
							const std::size_t ITERATIONS = 2 == argc ?
									static_cast< std::size_t >(std::atoi( argv[1] )) :
									10u;
							env.introduce_coop(
									so_5::disp::active_obj::create_disp_binder(
											"active_obj" ),
									[&]( so_5::coop_t & coop ) {
										coop.make_agent< a_runner_t >( ITERATIONS );
									} );
						},
						[]( so_5::environment_params_t & p ) {
							p.add_named_dispatcher(
								"active_obj",
								so_5::disp::active_obj::create_disp() );
						} );
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}

