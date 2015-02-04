/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.3
 * \file
 * \brief Forward definitions for subscription storage related stuff.
 */

#pragma once

#include <memory>
#include <functional>

#include <so_5/h/declspec.hpp>

namespace so_5
{

namespace rt
{

class agent_t;

namespace impl
{

class subscription_storage_t;

/*!
 * \since v.5.5.3
 * \brief Typedef for unique pointer to subscription_storage object.
 */
using subscription_storage_unique_ptr_t =
		std::unique_ptr< subscription_storage_t >;

} /* namespace impl */

/*!
 * \since v.5.5.3
 * \brief Type of subscription_storage factory.
 */
using subscription_storage_factory_t =
		std::function<
				impl::subscription_storage_unique_ptr_t( agent_t * ) >;

/*!
 * \since v.5.5.3
 * \brief Factory for default subscription storage object.
 */
SO_5_FUNC subscription_storage_factory_t
default_subscription_storage_factory();

/*!
 * \since v.5.5.3
 * \brief Factory for default subscription storage based on std::unordered_map.
 */
SO_5_FUNC subscription_storage_factory_t
hash_table_based_subscription_storage_factory();

/*!
 * \since v.5.5.3
 * \brief Factory for subscription storage based on std::vector.
 */
SO_5_FUNC subscription_storage_factory_t
vector_based_subscription_storage_factory(
	//! Initial storage capacity.
	std::size_t initial_capacity );

/*!
 * \since v.5.5.3
 * \brief Factory for subscription storage based on std::map.
 */
SO_5_FUNC subscription_storage_factory_t
map_based_subscription_storage_factory();

/*!
 * \since v.5.5.3
 * \brief Factory for adaptive subscription storage.
 *
 * This storage will use vector-based storage for small amount
 * of subscriptions. And map-based storage for large amount
 * of subscriptions.
 */
SO_5_FUNC subscription_storage_factory_t
adaptive_subscription_storage_factory(
	//! Threshold for switching from small storage to the large one
	//! (and back from the large to the small in the case of subscription
	//! deletion).
	std::size_t threshold );

/*!
 * \since v.5.5.3
 * \brief Factory for adaptive subscription storage.
 *
 * This storage will use storage created by \a small_storage_factory for
 * small amount of subscriptions. And storage created by
 * \a large_storage_factory for large amount of subscriptions.
 */
SO_5_FUNC subscription_storage_factory_t
adaptive_subscription_storage_factory(
	//! Threshold for switching from small storage to the large one
	//! (and back from the large to the small in the case of subscription
	//! deletion).
	std::size_t threshold,
	//! A factory for creating small storage.
	const subscription_storage_factory_t & small_storage_factory,
	//! A factory for creating large storage.
	const subscription_storage_factory_t & large_storage_factory );

} /* namespace rt */

} /* namespace so_5 */

