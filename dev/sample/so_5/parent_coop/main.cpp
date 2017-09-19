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
struct task_result
{
	unsigned int m_id;
};

// Child finished to do his task.
class task_completed : public so_5::signal_t {}; 

// Child agent.
/*
	This agent will be created in his own cooperation.
	Agent does some task, sends a result to the parent agent and closes down.
*/
class a_child_t : public so_5::agent_t
{
	public:
		a_child_t(
			context_t ctx,
			so_5::mbox_t result_mbox,
			unsigned int task_id ) 
			:	so_5::agent_t( ctx )
			,	m_result_mbox( std::move(result_mbox) )
			,	m_task_id( task_id )
		{}

		virtual ~a_child_t() override
		{
			std::cout << "Child: agent of the task " << m_task_id
					<< " has destroyed." << std::endl;
		}

		// Definition of the agent for SObjectizer.
		virtual void so_define_agent() override
		{
			so_subscribe_self().event< task_completed >( &a_child_t::evt_task_completed );
		}

		// A reaction to start of work in SObjectizer.
		virtual void so_evt_start() override
		{
			std::cout << "Child: has started to do task " << m_task_id << std::endl;

			so_5::send_delayed< task_completed >( *this, std::chrono::seconds( 1 ) ); 
		}

		virtual void so_evt_finish() override
		{
			std::cout << "Child: has finished, task " << m_task_id << std::endl;
		}

		//! Child has completed the task.
		void evt_task_completed()
		{
			std::cout << "Child: has completed his task " << m_task_id << std::endl;

			// Send information about result to the parent agent.
			so_5::send< task_result >( m_result_mbox, m_task_id );

			// Deregister child cooperation and close 
			// down activity of this child instance.
			so_deregister_agent_coop_normally();
		}

	private:
		// Result mbox.
		const so_5::mbox_t m_result_mbox; 

		// Task ID.
		const unsigned int m_task_id;
};

// Parent agent in his parent cooperation.
class a_parent_t : public so_5::agent_t
{
	public:
		a_parent_t( context_t ctx ) :	so_5::agent_t( ctx )
		{}

		virtual ~a_parent_t() override
		{
			std::cout << "Parent: agent has destroyed." << std::endl;
		}

		// Definition of the agent for SObjectizer.
		virtual void so_define_agent() override
		{
			so_subscribe_self().event( &a_parent_t::evt_task_result );
		}

		// A reaction to start of work in SObjectizer.
		virtual void so_evt_start() override
		{
			std::cout << "Parent: agent has started." << std::endl;

			// Our cooperation has created. We may start a child agent work.
			start_child( 0 );
		}

		virtual void so_evt_finish() override
		{
			std::cout << "Parent: agent has finished." << std::endl;
		}

		// Task result received from child-agent.
		void evt_task_result( const task_result & evt )
		{
			std::cout << "Parent: task result " << evt.m_id << " is received." << std::endl;

			// We will start to do a next task.
			start_child( evt.m_id + 1 );
		}

	private:
		// Starts child to solve a task number ID.
		void start_child( unsigned int id )
		{
			std::cout << "Parent: starting a child to do task " << id << std::endl;

			// Creating a child cooperation.
			so_5::introduce_child_coop( *this, so_5::autoname,
				[&]( so_5::coop_t & coop ) {
					// Adding agents to the cooperation.
					coop.make_agent< a_child_t >( so_direct_mbox(), id );
				} );
		}
};

int main()
{
	try
	{
		so_5::launch( []( so_5::environment_t & env )
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
