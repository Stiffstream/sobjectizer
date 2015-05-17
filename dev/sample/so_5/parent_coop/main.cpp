/*
 * Parent-child cooperations sample.
 *
 * The sample shows:
 * - how to create a child-cooperation (cooperation, which has some parent
 *   cooperation).
 * - auto-deregistration of the child-cooperation if the parent cooperation is
 *   deregistering.
 *
 * Parent agent creates a child agent, which do some task. 
 * If parent cooperation is deregistering, the child cooperation will be
 * deregistered automatically.
 */

#include <iostream>
#include <sstream>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Result of task which will be send to a parent agent.
class task_result_t : public so_5::rt::message_t
{
	public:
		task_result_t( unsigned int id ) : m_id( id ) {}

		unsigned int
		id() const
		{
			return m_id;
		}

	private:
		unsigned int m_id;
};

// Child finished to do his task.
class task_completed_t : public so_5::rt::signal_t {}; 

// Child agent.
/*
	This agent will be created in his own cooperation.
	Agent does some task, sends a result to the parent agent and closes down.
*/
class a_child_t : public so_5::rt::agent_t
{
	public:
		a_child_t( so_5::rt::environment_t & env,
			const so_5::rt::mbox_t & result_mbox,
			unsigned int task_id ) 
		:	so_5::rt::agent_t( env )
		,	m_result_mbox( result_mbox )
		,	m_task_id( task_id )
		{}

		virtual ~a_child_t()
		{
			std::cout << "Child: agent of the task " << m_task_id
					<< " has destroyed." << std::endl;
		}

		// Definition of the agent for SObjectizer.
		virtual void
		so_define_agent() override
		{
			so_subscribe_self()
				.event( &a_child_t::evt_task_completed );
		}

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start() override
		{
			std::cout << "Child: has started to do task " << m_task_id
					<< std::endl;

			so_5::send_delayed_to_agent< task_completed_t >(
					*this,
					// One second delay.
					std::chrono::seconds( 1 ) ); 
		}

		virtual void
		so_evt_finish() override
		{
			std::cout << "Child: has finished, task " << m_task_id << std::endl;
		}

		//! Child has completed the task.
		void
		evt_task_completed(
			const so_5::rt::event_data_t< task_completed_t > & )
		{
			std::cout << "Child: has completed his task " << m_task_id
					<< std::endl;

			// Send information about result to the parent agent.
			so_5::send< task_result_t >( m_result_mbox, m_task_id );

			// Deregister child cooperation and close 
			// down activity of this child instance.
			so_deregister_agent_coop_normally();
		}

	private:
		// Result mbox.
		const so_5::rt::mbox_t m_result_mbox; 

		// Task ID.
		const unsigned int m_task_id;
};

// Parent agent in his parent cooperation.
class a_parent_t : public so_5::rt::agent_t
{
	public:
		a_parent_t( so_5::rt::environment_t & env ) 
		:	so_5::rt::agent_t( env )
		{}

		virtual ~a_parent_t()
		{
			std::cout << "Parent: agent has destroyed." << std::endl;
		}

		// Definition of the agent for SObjectizer.
		virtual void
		so_define_agent() override
		{
			so_subscribe_self()
				.event( &a_parent_t::evt_task_result );
		}

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start() override
		{
			std::cout << "Parent: agent has started." << std::endl;

			// Our cooperation has created. We may start a child agent work.
			start_child( 0 );
		}

		virtual void
		so_evt_finish() override
		{
			std::cout << "Parent: agent has finished." << std::endl;
		}

		// Task result received from child-agent.
		void
		evt_task_result(
			const task_result_t & task_result )
		{
			std::cout << "Parent: task result " << task_result.id()
				<< " is received." << std::endl;

			// We will start to do a next task.
			start_child( task_result.id() + 1 );
		}

	private:

		//! Starts child to solve a task number ID.
		void
		start_child( unsigned int id )
		{
			std::cout << "Parent: starting a child to do task " << id << std::endl;

			// Creating a child cooperation.
			so_5::rt::introduce_child_coop( *this, so_5::autoname,
				[&]( so_5::rt::agent_coop_t & coop ) {
					// Adding agents to the cooperation.
					coop.make_agent< a_child_t >( so_direct_mbox(), id );
				} );
		}

		// Agent mbox.
		so_5::rt::mbox_t m_self_mbox;
};

int
main()
{
	try
	{
		so_5::launch( []( so_5::rt::environment_t & env )
			{
				// Registering the parent cooperation.
				env.register_agent_as_coop( "coop",
						env.make_agent< a_parent_t >() );

				// Give some time to the agents.
				std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
				env.stop();
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
