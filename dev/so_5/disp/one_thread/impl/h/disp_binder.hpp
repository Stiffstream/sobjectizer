/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Interface for binding agent to a dispatcher with the single
	working thread.
*/

#if !defined( _SO_5__DISP__ONE_THREAD__IMPL__DISP_BINDER_HPP_ )
#define _SO_5__DISP__ONE_THREAD__IMPL__DISP_BINDER_HPP_

#include <so_5/rt/h/disp_binder.hpp>
#include <so_5/rt/h/agent.hpp>
#include <so_5/rt/impl/h/so_environment_impl.hpp>

namespace so_5
{

namespace disp
{

namespace one_thread
{

namespace impl
{

//! Agent dispatcher binder.
class disp_binder_t
	:
		public so_5::rt::disp_binder_t
{
	public:
		explicit disp_binder_t(
			const std::string & disp_name );
		virtual ~disp_binder_t();


		virtual so_5::rt::disp_binding_activator_t
		bind_agent(
			so_5::rt::so_environment_t & env,
			so_5::rt::agent_ref_t agent_ref );

		virtual void
		unbind_agent(
			so_5::rt::so_environment_t & env,
			so_5::rt::agent_ref_t agent_ref );

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
			so_5::rt::agent_ref_t agent_ref );
};

} /* namespace impl */

} /* namespace one_thread */

} /* namespace disp */

} /* namespace so_5 */

#endif

