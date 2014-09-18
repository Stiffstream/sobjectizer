/*
	SObjectizer 5.
*/

/*!
	\file
	\brief An addition layer for the SObjectizer Environment definition.
*/

#if !defined( _SO_5__RT__SO_LAYER_HPP_ )
#define _SO_5__RT__SO_LAYER_HPP_

#include <memory>
#include <map>
#include <typeindex>

#include <so_5/h/declspec.hpp>
#include <so_5/h/ret_code.hpp>

namespace so_5
{

namespace rt
{

class so_environment_t;

namespace impl
{

class layer_core_t;

} /* namespace impl */


//
// so_layer_t
//

//! An interface of the additional SObjectizer Environment layer.
/*!
*/
class SO_5_TYPE so_layer_t
{
		friend class impl::layer_core_t;
	public:
		so_layer_t();
		virtual ~so_layer_t();


		//! Start hook.
		/*!
		 * The default implementation do nothing.
		 */
		virtual void
		start();

		//! Shutdown signal hook.
		/*!
		 * The default implementation do nothing.
		 */
		virtual void
		shutdown();

		//! Waiting for the complete shutdown of a layer.
		/*!
		 * The default implementation do nothing and returned immediately.
		*/
		virtual void
		wait();

	protected:
		//! Access to the SObjectizer Environment.
		/*!
		 * Throws an exception if a layer is not bound to
		 * the SObjectizer Environment.
		 */
		so_environment_t &
		so_environment();

	private:
		//! Bind layer to the SObjectizer Environment.
		void
		bind_to_environment( so_environment_t * env );

		//! SObjectizer Environment to which layer is bound.
		/*!
		 * Pointer has the actual value only after binding 
		 * to the SObjectizer Environment.
		 */
		so_environment_t * m_so_environment;
};

//! Typedef for the layer's autopointer.
typedef std::unique_ptr< so_layer_t > so_layer_unique_ptr_t;

//! Typedef for the layer's smart pointer.
typedef std::shared_ptr< so_layer_t > so_layer_ref_t;

//! Typedef for rhe map from a layer typeid to the layer.
typedef std::map< std::type_index, so_layer_ref_t > so_layer_map_t;

} /* namespace rt */

} /* namespace so_5 */

#endif

