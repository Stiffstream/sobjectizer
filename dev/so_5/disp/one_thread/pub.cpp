/*
	SObjectizer 5.
*/

#include <so_5/disp/one_thread/h/pub.hpp>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/environment.hpp>

#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

namespace so_5
{

namespace disp
{

namespace one_thread
{

namespace impl
{

//
// dispatcher_t
//

/*!
	\brief A dispatcher with the single working thread and an event queue.
*/
class dispatcher_t : public so_5::rt::dispatcher_t
{
	public:
		//! \name Implementation of so_5::rt::dispatcher methods.
		//! \{
		virtual void
		start()
		{
			m_work_thread.start();
		}

		virtual void
		shutdown()
		{
			m_work_thread.shutdown();
		}

		virtual void
		wait()
		{
			m_work_thread.wait();
		}
		//! \}

		/*!
		 * \since v.5.4.0
		 * \brief Get a binding information for an agent.
		 */
		so_5::rt::event_queue_t *
		get_agent_binding()
		{
			return m_work_thread.get_agent_binding();
		}

	private:
		//! Working thread for the dispatcher.
		so_5::disp::reuse::work_thread::work_thread_t m_work_thread;
};

//
// disp_binder_t
//

//! Agent dispatcher binder.
class disp_binder_t : public so_5::rt::disp_binder_t
{
	public:
		explicit disp_binder_t(
			const std::string & disp_name )
			:	m_disp_name( disp_name )
		{}

		virtual so_5::rt::disp_binding_activator_t
		bind_agent(
			so_5::rt::environment_t & env,
			so_5::rt::agent_ref_t agent_ref )
		{
			if( m_disp_name.empty() )
				return make_agent_binding(
						&( env.query_default_dispatcher() ),
						std::move( agent_ref ) );
			else
				return make_agent_binding( 
					env.query_named_dispatcher( m_disp_name ).get(),
					std::move( agent_ref ) );
		}

		virtual void
		unbind_agent(
			so_5::rt::environment_t & env,
			so_5::rt::agent_ref_t agent_ref )
		{
		}

	private:
		//! Name of the dispatcher to be bound to.
		/*!
		 * Empty name means usage of default dispatcher.
		 */
		const std::string m_disp_name;

		/*!
		 * \since v.5.4.0
		 * \brief Make binding to the dispatcher specified.
		 */
		so_5::rt::disp_binding_activator_t
		make_agent_binding(
			so_5::rt::dispatcher_t * disp,
			so_5::rt::agent_ref_t agent )
		{
			// If the dispatcher is found then the object should be bound to it.
			if( !disp )
				SO_5_THROW_EXCEPTION( rc_named_disp_not_found,
						"dispatcher with name \"" + m_disp_name + "\" not found" );

			// It should be exactly our dispatcher.
			dispatcher_t * d = dynamic_cast< dispatcher_t * >( disp );

			if( nullptr == d )
				SO_5_THROW_EXCEPTION( rc_disp_type_mismatch,
					"disp type mismatch for disp \"" + m_disp_name +
							"\", expected one_thread disp" );

			return [agent, d]() {
				agent->so_bind_to_dispatcher( *(d->get_agent_binding()) );
			};
		}
};

} /* namespace impl */

SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp()
{
	return so_5::rt::dispatcher_unique_ptr_t( new impl::dispatcher_t );
}

SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder( const std::string & disp_name )
{
	return so_5::rt::disp_binder_unique_ptr_t(
		new impl::disp_binder_t( disp_name ) );
}

} /* namespace one_thread */

} /* namespace disp */

namespace rt
{

SO_5_FUNC disp_binder_unique_ptr_t
create_default_disp_binder()
{
	// Dispatcher with empty name means default dispatcher.
	return so_5::disp::one_thread::create_disp_binder( std::string() );
}

} /* namespace rt */

} /* namespace so_5 */

