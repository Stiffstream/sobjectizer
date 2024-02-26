/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.3
 *
 * \file
 * \brief Forward definitions for subscription storage related stuff.
 */

#pragma once

#include <so_5/declspec.hpp>

#include <so_5/fwd.hpp>

#include <memory>
#include <functional>

namespace so_5
{

namespace impl
{

class subscription_storage_t;

/*!
 * \since
 * v.5.5.3
 *
 * \brief Typedef for unique pointer to subscription_storage object.
 */
using subscription_storage_unique_ptr_t =
		std::unique_ptr< subscription_storage_t >;

} /* namespace impl */

/*!
 * \since
 * v.5.5.3
 *
 * \brief Type of subscription_storage factory.
 */
using subscription_storage_factory_t =
		std::function<
				impl::subscription_storage_unique_ptr_t() >;

/*!
 * \brief Factory for default subscription storage object.
 *
 * \note Creates adaptive storage with vector-based storage for
 * small amount of subscriptions and map-based storage for
 * large amount of subscriptions.
 *
 * \par More about subscription storage tuning
 * See \ref so_5_5_3__subscr_storage_selection for more details about selection
 * of appropriate subscription storage type.
 *
 * \since v.5.5.3
 */
SO_5_FUNC subscription_storage_factory_t
default_subscription_storage_factory();

/*!
 * \brief Factory for default subscription storage based on std::unordered_map.
 *
 * \note This storage is efficient only in the cases of very large amount
 * of subscriptions (from several hundreds to thousands). For smaller amounts
 * map-based and vector-based storages are more appropriate.
 *
 * \par More about subscription storage tuning
 * See \ref so_5_5_3__subscr_storage_selection for more details about selection
 * of appropriate subscription storage type.
 *
 * \since v.5.5.3
 */
SO_5_FUNC subscription_storage_factory_t
hash_table_based_subscription_storage_factory();

/*!
 * \brief Factory for subscription storage based on unsorted std::vector.
 *
 * \note Uses very simple working scheme: all subscriptions are stored in the
 * linear vector and simpliest linear search is used for seaching and
 * manipulating of subscriptions. Because of that vector-based storage must be
 * used only with very small amount of subscriptions.
 *
 * \par More about subscription storage tuning
 * See \ref so_5_5_3__subscr_storage_selection for more details about selection
 * of appropriate subscription storage type.
 *
 * \since v.5.5.3
 */
SO_5_FUNC subscription_storage_factory_t
vector_based_subscription_storage_factory(
	//! Initial storage capacity.
	std::size_t initial_capacity );

/*!
 * \brief Factory for subscription storage based on std::map.
 *
 * \note Uses std::map as an underlying storage. And very efficient when count
 * of subscription in measured in dozens. For very large amount of
 * subscriptions the hash-table-based storage is more efficient.
 *
 * \par More about subscription storage tuning
 * See \ref so_5_5_3__subscr_storage_selection for more details about selection
 * of appropriate subscription storage type.
 *
 * \since v.5.5.3
 */
SO_5_FUNC subscription_storage_factory_t
map_based_subscription_storage_factory();

/*!
 * \brief Factory for adaptive subscription storage.
 *
 * \par Description
 * This storage will use vector-based storage for small amount
 * of subscriptions. And map-based storage for large amount
 * of subscriptions.
 *
 * \par More about subscription storage tuning
 * See \ref so_5_5_3__subscr_storage_selection for more details about selection
 * of appropriate subscription storage type.
 *
 * \since v.5.5.3
 */
SO_5_FUNC subscription_storage_factory_t
adaptive_subscription_storage_factory(
	//! Threshold for switching from small storage to the large one
	//! (and back from the large to the small in the case of subscription
	//! deletion).
	std::size_t threshold );

/*!
 * \brief Factory for adaptive subscription storage.
 *
 * \par Description
 * This storage will use storage created by \a small_storage_factory for
 * small amount of subscriptions. And storage created by
 * \a large_storage_factory for large amount of subscriptions.
 *
 * \par Cascading of storages
 * This function can be used for creating more complex cascades of
 * several subscription storages. For example, the following code creates
 * adaptive storage with three underlying objects. The first will be used
 * for 0..10 subscriptions. The seconds will be used for 11..100 subscriptions.
 * And the third for 100+ subscriptions:
\code
so_5::adaptive_subscription_storage_factory(
	// First threshold for switching from the first storage to the second.
	10,
	// First storage -- simple vector-based.
	so_5::vector_based_subscription_storage_factory(10),
	// There will be another adaptive storage.
	so_5::adaptive_subscription_storage_factory(
		// Second threshold for switching from the second storage to the third.
		100,
		// Second storage.
		so_5::map_based_subscription_storage_factory(),
		// Third storage.
		so_5::hash_table_based_subscription_storage_factory() ) );
\endcode
 *
 *
 * \par More about subscription storage tuning
 * See \ref so_5_5_3__subscr_storage_selection for more details about selection
 * of appropriate subscription storage type.
 *
 * \since v.5.5.3
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

/*!
 * \brief Factory for subscription storage based on sorted std::vector.
 *
 * The capacity of the storage is not fixed, the underlaying vector will
 * grow as needed (note that the current implementation never shrinks it down).
 *
 * \note
 * Uses very simple working scheme: all subscriptions are stored in a
 * sorted vector (aka "flat_set") and binary search is used for seaching and
 * manipulating of subscriptions.
 *
 * \since v.5.5.3
 */
SO_5_FUNC subscription_storage_factory_t
flat_set_based_subscription_storage_factory(
	//! Initial storage capacity.
	std::size_t initial_capacity );

} /* namespace so_5 */

