/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A definition of an utility class for work with layers.
*/

#pragma once

#include <vector>
#include <typeindex>
#include <mutex>

#include <so_5/h/exception.hpp>

#include <so_5/rt/h/so_layer.hpp>

namespace so_5
{

class environment_t;

namespace impl
{

//
// typed_layer_ref_t
//

/*!
 * \brief A special wrapper to store a layer with its type.
 */
struct typed_layer_ref_t
{
	typed_layer_ref_t();
	typed_layer_ref_t( const layer_map_t::value_type & v );
	typed_layer_ref_t(
		const std::type_index & type,
		const layer_ref_t & layer );

	bool
	operator < ( const typed_layer_ref_t & tl ) const;

	//! Layer type.
	std::type_index m_true_type;

	//! Layer itself.
	layer_ref_t m_layer;
};

//! Typedef for typed_layer_ret container.
typedef std::vector< typed_layer_ref_t > so_layer_list_t;

//
// layer_core_t
//

//! An utility class for working with layers.
/*!
 * There are two groups of layers:
 * \li \c default layers. They are known before the SObjectizer Run-Time started
 * and are retreived from SObjectizer Environment params.
 * These layers must be passed to the layer_core_t constructor;
 * \li \c extra layers. They are added when the SObjectizer Envoronment works.
 *
 * The main difference between them is that default layers start when
 * SObjectizer Envoronment is not working. They are started during 
 * the SObjectizer initialization routine.
 *
 * The extra layer is started when it is added to the SObjectizer Envoronment.
 *
 * During the SObjectizer Environment shutdown extra layers must be shutdowned
 * before default layers.
 * 
 * If the SObjectizer Environment is started several times then 
 * the set of default layers can't be changed.
 * 
 * However, extra layers have a different life cycle. 
 * They must be formed from scratch at every start of the SObjectizer Environment.
 * When the SObjectizer Envoronment stops, it stops and deletes all of extra layers.
 * The extra layer can't be removed before stop of the SObjectizer Environment.
 */
class layer_core_t
{
	public:
		layer_core_t(
			//! SObjectizer Environment to work with.
			environment_t & env,
			//! Layers which are known before SObjectizer start.
			const layer_map_t & so_layers );

		//! Get a layer.
		/*!
		 * \retval nullptr if layer is not found.
		 */
		layer_t *
		query_layer(
			const std::type_index & type ) const;

		//! Start all layers.
		void
		start();

		/*!
		 * \since
		 * v.5.2.0
		 *
		 * \brief Shutdown all layers and wait for full stop of them.
		 */
		void
		finish();

		//! Add an extra layer.
		void
		add_extra_layer(
			const std::type_index & type,
			const layer_ref_t & layer );

	private:
		//! SObjectizer Environment to work with.
		environment_t & m_env;

		//! Default layers.
		/*!
		 * Value is set in the constructor and will never change.
		 */
		so_layer_list_t m_default_layers;

		//! Object lock for the extra layers data.
		mutable std::mutex m_extra_layers_lock;

		//! Extra layers.
		so_layer_list_t m_extra_layers;

		//! Shutdown extra layers.
		void
		shutdown_extra_layers();

		//! Blocking wait for the complete shutdown of all extra layers.
		/*!
		 * All extra layers are destroyed.
		 */
		void
		wait_extra_layers();

		//! Shutdown default layers.
		void
		shutdown_default_layers();

		//! Blocking wait for the complete shutdown of all default layers.
		void
		wait_default_layers();
};

} /* namespace impl */

} /* namespace so_5 */
