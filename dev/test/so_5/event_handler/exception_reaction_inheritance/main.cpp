/*
 * A test for checking exception reaction inheritance from
 * coop, parent_coop and from so_environment.
 */

#include <iostream>
#include <stdexcept>

#include <so_5/all.hpp>

#include "../../svc/a_time_sentinel.hpp"

struct msg_test_signal : public so_5::signal_t {};

class a_test_t
	:	public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			so_5::environment_t & env,
			const so_5::mbox_t & self_mbox )
			:	base_type_t( env )
			,	m_self_mbox( self_mbox )
		{}

		void
		so_define_agent()
		{
			so_subscribe( m_self_mbox ).event< msg_test_signal >( [] {
					throw std::runtime_error( "Exception from a_test_t!" );
				} );
		}

	private :
		const so_5::mbox_t m_self_mbox;
};

class a_parent_t
	:	public so_5::agent_t
{
	public :
		a_parent_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
		{}

		virtual void
		so_evt_start()
		{
			auto child = so_environment().create_coop(
					so_coop_name() + "::child" );
			child->set_parent_coop_name( so_coop_name() );

			auto mbox = so_environment().create_mbox();
			child->add_agent( new a_test_t( so_environment(), mbox ) );

			so_environment().register_coop( std::move(child) );

			mbox->deliver_signal< msg_test_signal >();
		}
};

void
init( so_5::environment_t & env )
{
	auto coop = env.create_coop( "test" );
	coop->add_agent( new a_parent_t( env ) );
	coop->add_agent( new a_time_sentinel_t( env ) );

	env.register_coop( std::move( coop ) );
}

int
main()
{
	try
	{
		so_5::launch( &init,
			[]( so_5::environment_params_t & params )
			{
				params.exception_reaction(
						so_5::shutdown_sobjectizer_on_exception );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

