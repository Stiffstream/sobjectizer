/*
 * A sample for the exception handler and cooperation notifications.
 */

#include <iostream>
#include <stdexcept>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// A class of an agent which will throw an exception.
class a_child_t :	public so_5::agent_t
{
	public :
		a_child_t( context_t ctx, bool should_throw )
			:	so_5::agent_t{ ctx }
			,	m_should_throw{ should_throw }
		{}

		virtual void so_evt_start() override
		{
			if( m_should_throw )
				throw std::runtime_error( "A child agent failure!" );
		}

	private :
		const bool m_should_throw;
};

// A class of parent agent.
class a_parent_t : public so_5::agent_t
{
	public :
		a_parent_t( context_t ctx )
			:	so_5::agent_t{ ctx }
			,	m_counter{ 0 }
			,	m_max_counter{ 3 }
		{}

		virtual void so_define_agent() override
		{
			so_default_state()
				.event( &a_parent_t::evt_child_created )
				.event( &a_parent_t::evt_child_destroyed );
		}

		void so_evt_start() override
		{
			register_child_coop();
		}

	private :
		int m_counter;
		const int m_max_counter;

		void evt_child_created(
			const so_5::msg_coop_registered & evt )
		{
			std::cout << "coop_reg: " << evt.m_coop_name << std::endl;

			if( m_counter >= m_max_counter )
				so_deregister_agent_coop_normally();

			// Otherwise should wait for cooperation shutdown.
		}

		void evt_child_destroyed(
			const so_5::msg_coop_deregistered & evt )
		{
			std::cout << "coop_dereg: " << evt.m_coop_name
				<< ", reason: " << evt.m_reason.reason() << std::endl;

			++m_counter;
			register_child_coop();
		}

		void register_child_coop()
		{
			using namespace so_5;

			introduce_child_coop( *this, "child",
				[this]( coop_t & coop )
				{
					coop.add_reg_notificator(
							make_coop_reg_notificator( so_direct_mbox() ) );
					coop.add_dereg_notificator(
							make_coop_dereg_notificator( so_direct_mbox() ) );
					coop.set_exception_reaction( deregister_coop_on_exception );

					coop.make_agent< a_child_t >( m_counter < m_max_counter );

					std::cout << "registering coop: " << coop.query_coop_name()
							<< std::endl;
				} );
		}
};

int main()
{
	try
	{
		so_5::launch(
			// The SObjectizer Environment initialization.
			[]( so_5::environment_t & env )
			{
				// Creating and registering a cooperation.
				env.register_agent_as_coop( "parent",
						env.make_agent< a_parent_t >() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

