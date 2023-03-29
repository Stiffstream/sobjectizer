/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Stuff for multi_sink_binding implementation.
 *
 * \since v.5.8.0
 */

#pragma once

#include <so_5/single_sink_binding.hpp>

#include <so_5/exception.hpp>
#include <so_5/ret_code.hpp>

#include <so_5/details/sync_helpers.hpp>
#include <so_5/details/rollback_on_exception.hpp>

#include <map>
#include <tuple>

namespace so_5
{

namespace multi_sink_binding_impl
{

/*!
 * \brief Type of container for bindings for messages.
 *
 * There could be just one binding for one message type.
 *
 * \since v.5.8.0
 */
using one_sink_bindings_t = std::map< std::type_index, single_sink_binding_t >;

/*!
 * \brief Type of container for bindings for one msink.
 *
 * There could be bindings for several message types for the same msink.
 *
 * \since v.5.8.0
 */
using one_mbox_bindings_t = std::map<
		msink_t,
		one_sink_bindings_t,
		so_5::impl::msink_less_comparator_t >;

/*!
 * \brief Type of container for bindings for messages from mboxes.
 *
 * Several msinks can be bound to one mbox.
 *
 * \since v.5.8.0
 */
using bindings_map_t = std::map< mbox_id_t, one_mbox_bindings_t >;

/*!
 * \brief Helper type for insertion into a std::map with automatic rollback.
 *
 * It's expected that \a Container is a std::map. It's expected that
 * Container::mapped_type is DefaultConstructible.
 *
 * If commit() isn't called and a new item was inserted to the map
 * in the constructor then that item will be erased in the destructor.
 *
 * See actual_binding_handler_t::do_actual_bind() for usage examples.
 *
 * \since v.5.8.0
 */
template< class Container >
class insertion_it_with_auto_erase_if_not_committed_t final
	{
		using iterator_type = typename Container::iterator;

		Container & m_container;
		iterator_type m_it;
		bool m_modified;
		bool m_commited{ false };

	public:
		/*!
		 * \brief Initializing constructor.
		 *
		 * Tries to find an item with \a k in \a container.
		 * If item isn't found then a new item with \a k as key will be inserted
		 * into \a container.
		 */
		insertion_it_with_auto_erase_if_not_committed_t(
			Container & container,
			typename Container::key_type const & k )
			:	m_container{ container }
			,	m_it{ container.find( k ) }
			{
				if( container.end() == m_it )
					{
						m_it = m_container.emplace( k, typename Container::mapped_type{} ).first;
						m_modified = true;
					}
				else
					m_modified = false;
			}

		insertion_it_with_auto_erase_if_not_committed_t(
			const insertion_it_with_auto_erase_if_not_committed_t & ) = delete;
		insertion_it_with_auto_erase_if_not_committed_t(
			insertion_it_with_auto_erase_if_not_committed_t && ) = delete;

		~insertion_it_with_auto_erase_if_not_committed_t() noexcept
			{
				if( m_modified && !m_commited )
					m_container.erase( m_it );
			}

		void
		commit() noexcept
			{
				m_commited = true;
			}

		[[nodiscard]]
		iterator_type
		operator->() const
			{
				return m_it;
			}

		/*!
		 * \retval true if new item was inserted in the constructor.
		 * \retval false if item was found in the container.
		 */
		[[nodiscard]]
		bool
		modified() const noexcept
			{
				return m_modified;
			}
	};

// Deduction guide for insertion_it_with_auto_erase_if_not_committed_t.
template<typename C, typename K>
insertion_it_with_auto_erase_if_not_committed_t(C &, const K&) ->
		insertion_it_with_auto_erase_if_not_committed_t<C>;

//FIXME: document this!
class actual_binding_handler_t
	{
		bindings_map_t m_bindings;

		template< typename Single_Sink_Modificator >
		void
		do_actual_bind(
			const std::type_index & msg_type,
			const mbox_t & from,
			const msink_t & dest,
			Single_Sink_Modificator && single_sink_modificator )
			{
				insertion_it_with_auto_erase_if_not_committed_t it_mbox{ m_bindings, from->id() };

				insertion_it_with_auto_erase_if_not_committed_t it_msink{ it_mbox->second, dest };

				insertion_it_with_auto_erase_if_not_committed_t it_msg{ it_msink->second, msg_type };
				// If new item wasn't inserted then it's an error.
				if( !it_msg.modified() )
					{
						SO_5_THROW_EXCEPTION(
								rc_evt_handler_already_provided,
								std::string{ "msink already subscribed to a message" } +
								"(mbox:'" + from->query_name() +
								"', msg_type:'" + msg_type.name() + "'" );
					}

				single_sink_modificator( msg_type, it_msg->second );

				it_msg.commit();
				it_msink.commit();
				it_mbox.commit();
			}

	public:
//FIXME: methods do_bind have too many similar lines, can they be refactored?
		// Can be used for signals.
		template< typename Msg >
		void
		do_bind(
			const mbox_t & from,
			const msink_t & dest )
			{
				do_actual_bind(
						message_payload_type< Msg >::subscription_type_index(),
						from,
						dest,
						[&](
							const std::type_index & msg_type,
							single_sink_binding_t & binding )
						{
							binding.bind_for_msg_type(
									msg_type,
									from,
									dest );
						} );
			}

		template< typename Msg >
		void
		do_bind(
			const mbox_t & from,
			const msink_t & dest,
			delivery_filter_unique_ptr_t delivery_filter )
			{
				// Msg can't be a signal!
				ensure_not_signal< Msg >();

				do_actual_bind(
						message_payload_type< Msg >::subscription_type_index(),
						from,
						dest,
						[&](
							const std::type_index & msg_type,
							single_sink_binding_t & binding )
						{
							binding.bind_for_msg_type(
									msg_type,
									from,
									dest,
									std::move(delivery_filter) );
						} );
			}

		template< typename Msg >
		void
		do_unbind(
			const mbox_t & from,
			const msink_t & dest ) noexcept
			{
				auto it_mbox = m_bindings.find( from->id() );
				if( it_mbox == m_bindings.end() )
					return;

				auto & msinks = it_mbox->second;
				auto it_msink = msinks.find( dest );
				if( it_msink == msinks.end() )
					return;

				const auto & msg_type =
						message_payload_type< Msg >::subscription_type_index();

				auto & msgs = it_msink->second;
				msgs.erase( msg_type );

				if( msgs.empty() )
					{
						msinks.erase( it_msink );
						if( msinks.empty() )
							{
								m_bindings.erase( it_mbox );
							}
					}
			}

		void
		do_unbind_all_for(
			const mbox_t & from,
			const msink_t & dest ) noexcept
			{
				auto it_mbox = m_bindings.find( from->id() );
				if( it_mbox == m_bindings.end() )
					return;

				auto & msinks = it_mbox->second;
				msinks.erase( dest );
				if( msinks.empty() )
					m_bindings.erase( it_mbox );
			}

		void
		do_clear() noexcept
			{
				m_bindings.clear();
			}
	};

} /* namespace multi_sink_binding_impl */

template< typename Lock_Type = std::mutex >
class multi_sink_binding_t
	:	protected so_5::details::lock_holder_detector< Lock_Type >::type
	,	protected multi_sink_binding_impl::actual_binding_handler_t
	{
	public:
		multi_sink_binding_t() = default;

		multi_sink_binding_t( const multi_sink_binding_t & ) = delete;
		multi_sink_binding_t &
		operator=( const multi_sink_binding_t & ) = delete;

		multi_sink_binding_t( multi_sink_binding_t && ) = delete;
		multi_sink_binding_t &
		operator=( multi_sink_binding_t && ) = delete;

		template< typename Msg >
		void
		bind(
			const mbox_t & from,
			const msink_t & dest )
			{
				this->lock_and_perform( [&]() {
						this->template do_bind< Msg >(
								from,
								dest );
					} );
			}

		template< typename Msg >
		void
		bind(
			const mbox_t & from,
			const msink_t & dest,
			//NOTE: delivery_filter can't be null!
			delivery_filter_unique_ptr_t delivery_filter )
			{
				so_5::low_level_api::ensure_not_null( delivery_filter );

				//FIXME: can delivery_filter be null here?
				this->lock_and_perform( [&]() {
						this->template do_bind< Msg >(
								from,
								dest,
								std::move(delivery_filter) );
					} );
			}

		//FIXME: document this!
		template< typename Msg, typename Lambda >
		void
		bind(
			const mbox_t & from,
			const msink_t & dest,
			Lambda && filter )
			{
				using namespace so_5::details::lambda_traits;

				using lambda_type = std::remove_reference_t< Lambda >;
				using argument_type =
						typename argument_type_if_lambda< lambda_type >::type;

				// For cases when Msg is mutable_msg<M>.
				sink_bindings_details::ensure_valid_argument_for_delivery_filter<
						typename so_5::message_payload_type<Msg>::payload_type,
						argument_type
					>();

				delivery_filter_unique_ptr_t filter_holder{
						new low_level_api::lambda_as_filter_t< lambda_type, argument_type >(
								std::move(filter) )
					};

				this->bind< Msg >( from, dest, std::move(filter_holder) );
			}

		template< typename Msg >
		void
		unbind(
			const mbox_t & from,
			const msink_t & dest ) noexcept
			{
				this->lock_and_perform( [&]() {
						this->template do_unbind< Msg >( from, dest );
					} );
			}

		void
		unbind_all_for(
			const mbox_t & from,
			const msink_t & dest ) noexcept
			{
				this->lock_and_perform( [&]() {
						this->do_unbind_all_for( from, dest );
					} );
			}

		void
		clear() noexcept
			{
				this->lock_and_perform( [&]() {
						this->do_clear();
					} );
			}
	};

} /* namespace so_5 */

