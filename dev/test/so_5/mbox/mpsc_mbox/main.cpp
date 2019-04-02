/*
 * A test for mpsc_mbox.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

struct msg_one : public so_5::signal_t {};
struct msg_two : public so_5::signal_t {};
struct msg_three : public so_5::signal_t {};
struct msg_four : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			so_5::environment_t & env,
			std::string & sequence )
			:	base_type_t( env )
			,	m_sequence( sequence )
		{
		}

		void
		so_define_agent() override
		{
			so_subscribe( so_direct_mbox() )
				.event( &a_test_t::evt_one );
			so_subscribe( so_direct_mbox() )
				.event( &a_test_t::evt_three );
			so_subscribe( so_direct_mbox() )
				.event( &a_test_t::evt_four );
		}

		void
		so_evt_start() override
		{
			so_5::send< msg_one >( *this );
			so_5::send< msg_two >( *this );
			so_5::send< msg_three >( *this );
		}

		void
		evt_one( mhood_t< msg_one > )
		{
			m_sequence += "e1:";
		}

		void
		evt_two( mhood_t< msg_two > )
		{
			m_sequence += "e2:";
		}

		void
		evt_three( mhood_t< msg_three > )
		{
			m_sequence += "e3:";

			so_drop_subscription( so_direct_mbox(), &a_test_t::evt_one );

			so_subscribe( so_direct_mbox() ).event( &a_test_t::evt_two );

			so_5::send< msg_one >( *this );
			so_5::send< msg_two >( *this );

			so_5::send< msg_four >( *this );
		}

		void
		evt_four( mhood_t< msg_four > )
		{
			m_sequence += "e4:";

			so_environment().stop();
		}

	private :
		std::string & m_sequence;
};

int
main()
{
	try
	{
		std::string sequence;

		so_5::launch(
			[&sequence]( so_5::environment_t & env )
			{
				auto coop = env.make_coop(
					so_5::disp::active_obj::make_dispatcher( env ).binder() );

				coop->make_agent< a_test_t >( std::ref(sequence) );

				env.register_coop( std::move( coop ) );
			} );

		const std::string expected = "e1:e3:e2:e4:";
		if( sequence != expected )
			throw std::runtime_error( "sequence mismatch! "
					"expected: '" + expected + "', actual: '"
					+ sequence + "'" );

	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

