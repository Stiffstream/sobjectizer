/*
	SObjectizer 5.
*/

/*!
	\file
	\brief An addition layer for the SObjectizer Environment definition.
*/

#pragma once

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>
#include <so_5/h/ret_code.hpp>

#include <so_5/rt/h/fwd.hpp>

#include <memory>
#include <map>
#include <typeindex>

namespace so_5
{

//
// so_layer_t
//

//! An interface of the additional SObjectizer Environment layer.
class SO_5_TYPE layer_t
{
		friend class impl::layer_core_t;

		// Note: clang-3.9 requires this on Windows platform.
		layer_t( const layer_t & ) = delete;
		layer_t( layer_t && ) = delete;
		layer_t & operator=( const layer_t & ) = delete;
		layer_t & operator=( layer_t && ) = delete;

	public:
		layer_t() = default;
		virtual ~layer_t() SO_5_NOEXCEPT = default;


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
		environment_t &
		so_environment();

	private:
		//! Bind layer to the SObjectizer Environment.
		void
		bind_to_environment( environment_t * env );

		//! SObjectizer Environment to which layer is bound.
		/*!
		 * Pointer has the actual value only after binding 
		 * to the SObjectizer Environment.
		 */
		environment_t * m_env{};
};

//! Typedef for the layer's autopointer.
using layer_unique_ptr_t = std::unique_ptr< layer_t >;

//! Typedef for the layer's smart pointer.
using layer_ref_t = std::shared_ptr< layer_t >;

//! Typedef for the map from a layer typeid to the layer.
using layer_map_t = std::map< std::type_index, layer_ref_t >;

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::layer_t instead.
 */
using so_layer_t = so_5::layer_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::layer_unique_ptr_t
 * instead.
 */
using so_layer_unique_ptr_t = so_5::layer_unique_ptr_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::layer_ref_t instead.
 */
using so_layer_ref_t = so_5::layer_ref_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::layer_map_t
 * instead.
 */
using so_layer_map_t = so_5::layer_map_t;

} /* namespace rt */

} /* namespace so_5 */

