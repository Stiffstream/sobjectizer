/*
	SObjectizer 5.
*/

#include <so_5/rt/h/disp_binder.hpp>

#include <so_5/rt/h/environment.hpp>

#include <so_5/h/spinlocks.hpp>

#include <so_5/h/stdcpp.hpp>

namespace so_5
{

namespace impl {

//
// pre5_5_19_default_disp_binder_t
//
/*!
 * \brief An implementation for default dispatcher binder
 * which is necessary for compatibility with versions prior to 5.5.19.
 *
 * \since
 * v.5.5.19
 */
class pre5_5_19_default_disp_binder_t final : public so_5::disp_binder_t
	{
	public :
		virtual disp_binding_activator_t
		bind_agent(
			environment_t & env,
			agent_ref_t agent ) override
			{
				// Actual binder must be created if necessary.
				{
					std::lock_guard< default_spinlock_t > lock( m_lock );
					if( !m_actual_binder )
						m_actual_binder = env.so_make_default_disp_binder();
				}
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
		//! Object lock.
		/*!
		 * Necessary to protect m_actual_binder value.
		 */
		default_spinlock_t m_lock;
		disp_binder_unique_ptr_t m_actual_binder;
	};

} /* namespace impl */

//! Create an instance of the default dispatcher binding.
SO_5_FUNC disp_binder_unique_ptr_t
create_default_disp_binder()
	{
		return stdcpp::make_unique< impl::pre5_5_19_default_disp_binder_t >();
	}

//
// make_default_disp_binder
//
SO_5_FUNC disp_binder_unique_ptr_t
make_default_disp_binder( environment_t & env )
	{
		return env.so_make_default_disp_binder();
	}

} /* namespace so_5 */

