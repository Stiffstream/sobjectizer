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

#include <so_5/rt/h/mbox.hpp>

#include <so_5/h/compiler_features.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>

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
						const abstract_message_box_t & a = *m_mbox;
						const abstract_message_box_t & b = *(o.m_mbox);
						return a < b || ( a == b && m_msg_type < o.m_msg_type );
					}
			};

		//! Type of filters map.
		using map_t = std::map< key_t, delivery_filter_unique_ptr_t >;

		//! Information about defined filters.
		map_t m_filters;

	public :
		//! Drop all defined filters.
		/*!
		 * Filters are removed from corresponding mboxes and destroyed.
		 */
		void
		drop_all( agent_t & owner ) SO_5_NOEXCEPT
			{
				for( auto & p : m_filters )
					p.first.m_mbox->drop_delivery_filter(
							p.first.m_msg_type,
							owner );

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
			agent_t & owner )
			{
				const key_t key{ mbox, msg_type };
				auto it = m_filters.find( key );
				if( it == m_filters.end() )
					{
						// There is no previous filter.
						// New filter must be added.
						auto ins_result = m_filters.emplace(
								map_t::value_type{ key, std::move( filter ) } );
						so_5::details::do_with_rollback_on_exception(
							[&] {
								mbox->set_delivery_filter(
										msg_type,
										*(ins_result.first->second),
										owner );
							},
							[&] {
								m_filters.erase( ins_result.first );
							} );
					}
				else
					{
						// Replace previous filter with new one.
						delivery_filter_unique_ptr_t old{ std::move( it->second ) };
						it->second = std::move( filter );

						// Mbox must change delivery filter too.
						so_5::details::do_with_rollback_on_exception(
							[&] {
								mbox->set_delivery_filter(
										msg_type,
										*(it->second),
										owner );
							},
							[&] {
								it->second = std::move( old );
							} );
					}
			}

		//! Remove delivery filter.
		void
		drop_delivery_filter(
			const mbox_t & mbox,
			const std::type_index & msg_type,
			agent_t & owner ) SO_5_NOEXCEPT
			{
				auto it = m_filters.find( key_t{ mbox, msg_type } );
				if( it != m_filters.end() )
					{
						mbox->drop_delivery_filter( msg_type, owner );
						m_filters.erase( it );
					}
			}
	};

} /* namespace impl */

} /* namespace so_5 */

