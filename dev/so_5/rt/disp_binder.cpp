/*
	SObjectizer 5.
*/

#include <so_5/rt/h/disp_binder.hpp>

#include <so_5/rt/h/environment.hpp>

#include <so_5/h/stdcpp.hpp>

namespace so_5
{

//
// disp_binder_t
//

disp_binder_t::disp_binder_t()
{
}

disp_binder_t::~disp_binder_t()
{
}

namespace impl {

//
// default_disp_binder_t
//
/*!
 * \brief An implementation for default dispatcher binder.
 *
 * It is created just to keep compatibility with previous versions
 * of SObjectizer.
 *
 * \since
 * v.5.5.19
 */
class default_disp_binder_t final : public so_5::disp_binder_t
	{
	public :
		virtual disp_binding_activator_t
		bind_agent(
			environment_t & env,
			agent_ref_t agent ) override
			{
				m_actual_binder = env.so_make_default_disp_binder();
				return m_actual_binder->bind_agent( env, std::move(agent) );
			}

		virtual void
		unbind_agent(
			environment_t & env,
			agent_ref_t agent ) override
			{
				m_actual_binder->unbind_agent( env, std::move(agent) );
			}

	private :
		disp_binder_unique_ptr_t m_actual_binder;
	};

} /* namespace impl */

//! Create an instance of the default dispatcher binding.
SO_5_FUNC disp_binder_unique_ptr_t
create_default_disp_binder()
	{
		return stdcpp::make_unique< impl::default_disp_binder_t >();
	}

} /* namespace so_5 */

