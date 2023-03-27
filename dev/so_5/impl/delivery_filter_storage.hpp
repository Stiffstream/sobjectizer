/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.5
 *
 * \file
 * \brief Definition of storage for delivery filters.
 */

#pragma once

#include <so_5/mbox.hpp>

#include <so_5/compiler_features.hpp>

#include <so_5/details/rollback_on_exception.hpp>

#include <functional>
#include <map>

namespace so_5 {

namespace impl {

//
// delivery_filter_storage_t
//
/*!
 * \since
 * v.5.5.5
 *
 * \brief Storage for message delivery filters.
 *
 * \attention This storage do not drop filters in the destructor.
 * It is because the reference to agent is necessary for filter dropping.
 * Storage do not have this reference.
 * Because of that it is necessary to drop all filters explicitely
 * by drop_all() method.
 */
class delivery_filter_storage_t
	{
		//! Type of key for filters map.
		struct key_t
			{
				//! Message mbox.
				mbox_t m_mbox;
				//! Message type.
				std::type_index m_msg_type;

				bool
				operator<( const key_t & o ) const
					{
						const auto id_a = m_mbox->id();
						const auto id_b = o.m_mbox->id();
						return id_a < id_b || ( id_a == id_b && m_msg_type < o.m_msg_type );
					}
			};

		//! Type of value for filters map.
		struct value_t
			{
				//! Delivery filter.
				/*!
				 * @note
				 * An instance of value_t owns the filter!
				 */
				delivery_filter_unique_ptr_t m_filter;

				//! Message sink for that the filter was set.
				/*!
				 * A reference to message_sink is used instead of raw (non-owning) pointer
				 * to indicate that a valid reference is required here (null is a valid
				 * value for a pointer).
				 *
				 * @note
				 * The lifetime of the sink is controlled elsewhere.
				 */
				std::reference_wrapper< abstract_message_sink_t > m_sink;

				value_t(
					delivery_filter_unique_ptr_t filter,
					so_5::outliving_reference_t< abstract_message_sink_t > sink )
					:	m_filter{ std::move(filter) }
					,	m_sink{ sink.get() }
					{}
			};

		//! Type of filters map.
		using map_t = std::map< key_t, value_t >;

		//! Information about defined filters.
		map_t m_filters;

	public :
		//! Drop all defined filters.
		/*!
		 * Filters are removed from corresponding mboxes and destroyed.
		 */
		void
		drop_all() noexcept
			{
				for( auto & [k, v] : m_filters )
					k.m_mbox->drop_delivery_filter( k.m_msg_type, v.m_sink.get() );

				m_filters.clear();
			}

		//! Set a delivery filter.
		/*!
		 * If there already is previous filter it will be destroyed.
		 */
		void
		set_delivery_filter(
			const mbox_t & mbox,
			const std::type_index & msg_type,
			delivery_filter_unique_ptr_t filter,
			so_5::outliving_reference_t< abstract_message_sink_t > owner )
			{
				const key_t key{ mbox, msg_type };
				auto it = m_filters.find( key );
				if( it == m_filters.end() )
					{
						// There is no previous filter.
						// New filter must be added.
						auto ins_result = m_filters.emplace(
								key,
								value_t{
										std::move( filter ),
										owner
								} );
						so_5::details::do_with_rollback_on_exception(
							[&] {
								auto & insertion_it = ins_result.first;
								mbox->set_delivery_filter(
										msg_type,
										*(insertion_it->second.m_filter),
										owner.get() );
							},
							[this, &ins_result] {
								auto & insertion_it = ins_result.first;
								m_filters.erase( insertion_it );
							} );
					}
				else
					{
						// Replace previous filter with new one.
						value_t old_value{ std::move(it->second) };
						it->second = value_t{ std::move( filter ), owner };

						// Mbox must change delivery filter too.
						so_5::details::do_with_rollback_on_exception(
							[&] {
								mbox->set_delivery_filter(
										msg_type,
										*(it->second.m_filter),
										owner.get() );
							},
							[&] {
								it->second = std::move(old_value);
							} );
					}
			}

		//! Remove delivery filter.
		void
		drop_delivery_filter(
			const mbox_t & mbox,
			const std::type_index & msg_type ) noexcept
			{
				auto it = m_filters.find( key_t{ mbox, msg_type } );
				if( it != m_filters.end() )
					{
						mbox->drop_delivery_filter(
								msg_type,
								it->second.m_sink.get() );
						m_filters.erase( it );
					}
			}
	};

} /* namespace impl */

} /* namespace so_5 */

