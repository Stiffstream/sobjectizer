/*
 * A test for massive usage of direct mboxes even when owners are
 * destroyed.
 */

#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include <so_5/all.hpp>

struct msg_ping : public so_5::rt::signal_t {};
struct msg_ack : public so_5::rt::signal_t {};

struct msg_child_agent_destroyed : public so_5::rt::signal_t {};

struct msg_next_iteration : public so_5::rt::signal_t {};

// A class of child agent.
class a_child_t
	:	public so_5::rt::agent_t
{
	public :
		a_child_t(
			so_5::rt::environment_t & env,
			const so_5::rt::mbox_ref_t & parent_mbox )
			:	so_5::rt::agent_t( env )
			,	m_parent_mbox( parent_mbox )
			{}
		~a_child_t()
			{
				m_parent_mbox->deliver_signal< msg_child_agent_destroyed >();
			}

		virtual void
		so_define_agent()
			{
				so_subscribe_self().event< msg_ping >( [=]() {
						m_parent_mbox->deliver_signal< msg_ack >();
					} );
			}

	private :
		const so_5::rt::mbox_ref_t m_parent_mbox;
};

// A class of parent agent.
class a_parent_t
	:	public so_5::rt::agent_t
{
	typedef so_5::rt::agent_t base_type_t;

	public :
		a_parent_t(
			so_5::rt::environment_t & env,
			int iterations )
			:	base_type_t( env )
			,	m_iterations_left( iterations )
			,	m_state( state_t::awaiting_creation )
			,	m_max_agents( 1000 )
			,	m_acks_received( 0 )
			,	m_destroy_received( 0 )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe_self().event( &a_parent_t::evt_child_created );

			so_subscribe_self().event( &a_parent_t::evt_child_destroyed );

			so_subscribe_self().event< msg_ack >( &a_parent_t::evt_ack );

			so_subscribe_self().event< msg_child_agent_destroyed >(
					&a_parent_t::evt_child_agent_destroyed );

			so_subscribe_self().event< msg_next_iteration >(
					&a_parent_t::evt_next_iteration );
		}

		void
		so_evt_start()
		{
			try_start_new_iteration();
		}

		void
		evt_child_created(
			const so_5::rt::msg_coop_registered & evt )
		{
			if( m_state != state_t::awaiting_creation )
				throw std::runtime_error( "expected awaiting_creation state!" );

			m_state = state_t::awaiting_acks;

			for( auto & m : m_child_mboxes )
				m->deliver_signal< msg_ping >();
		}

		void
		evt_child_destroyed(
			const so_5::rt::msg_coop_deregistered & evt )
		{
			if( m_state != state_t::awaiting_destroying )
				throw std::runtime_error( "msg_coop_deregistered when "
						"m_state != state_t::awaiting_destroying" );

			if( m_destroy_received != m_max_agents )
				throw std::runtime_error( "not all agents destroyed before "
						"msg_coop_deregistered received" );

			// This action must not lead to any damages (like memory leaks).
			for( auto & m : m_child_mboxes )
				m->deliver_signal< msg_ping >();

			--m_iterations_left;
			try_start_new_iteration();
		}

		void
		evt_ack()
		{
			if( m_state != state_t::awaiting_acks )
				throw std::runtime_error( "msg_ack when "
						"m_state != state_t::awaiting_acks" );

			++m_acks_received;
			if( m_acks_received == m_max_agents )
			{
				m_state = state_t::awaiting_destroying;
				so_environment().deregister_coop( "child",
						so_5::rt::dereg_reason::normal );
			}
		}
		
		void
		evt_child_agent_destroyed()
		{
			if( m_state != state_t::awaiting_destroying )
				throw std::runtime_error( "msg_child_agent_destroyed when "
						"m_state != state_t::awaiting_destroying" );

			++m_destroy_received;
		}

		void
		evt_next_iteration()
		{
			try_start_new_iteration();
		}

	private :
		int m_iterations_left;

		enum class state_t
			{
				awaiting_creation,
				awaiting_acks,
				awaiting_destroying
			};
		state_t m_state;

		const std::size_t m_max_agents;
		std::size_t m_acks_received;
		std::size_t m_destroy_received;

		std::vector< so_5::rt::mbox_ref_t > m_child_mboxes;

		void
		try_start_new_iteration()
		{
			if( m_iterations_left <= 0 )
			{
				std::cout << "COMPLETED!" << std::endl;

				so_environment().stop();
				return;
			}

			std::cout << m_iterations_left << " iterations left...\r"
				<< std::flush;

			m_state = state_t::awaiting_creation;
			m_acks_received = 0;
			m_destroy_received = 0;

			m_child_mboxes = std::vector< so_5::rt::mbox_ref_t >();
			m_child_mboxes.reserve( m_max_agents );

			auto coop = so_environment().create_coop( "child" );
			coop->set_parent_coop_name( so_coop_name() );
			coop->add_reg_notificator(
					so_5::rt::make_coop_reg_notificator( so_direct_mbox() ) );
			coop->add_dereg_notificator(
					so_5::rt::make_coop_dereg_notificator( so_direct_mbox() ) );

			for( std::size_t i = 0; i != m_max_agents; ++i )
			{
				std::unique_ptr< so_5::rt::agent_t > agent(
						new a_child_t(
								so_environment(),
								so_direct_mbox() ) );
				m_child_mboxes.push_back( agent->so_direct_mbox() );

				coop->add_agent( std::move( agent ) );
			}

			so_environment().register_coop( std::move( coop ) );
		}
};

int
main( int argc, char ** argv )
{
	try
	{
		const int iterations = argc == 2 ? std::atoi( argv[ 1 ] ) : 100;

		so_5::launch(
			[iterations]( so_5::rt::environment_t & env )
			{
				env.register_agent_as_coop( "parent",
					new a_parent_t( env, iterations ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

