/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Interface for binding agent to an active object dispatcher.
*/

#if !defined( _SO_5__DISP__ACTIVE_OBJ__IMPL__DISP_BINDER_HPP_ )
#define _SO_5__DISP__ACTIVE_OBJ__IMPL__DISP_BINDER_HPP_

#include <so_5/rt/h/disp_binder.hpp>
#include <so_5/rt/h/agent.hpp>
#include <so_5/rt/impl/h/so_environment_impl.hpp>

namespace so_5
{

namespace disp
{

namespace active_obj
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
		const std::string m_disp_name;
};

} /* namespace impl */

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */

#endif
