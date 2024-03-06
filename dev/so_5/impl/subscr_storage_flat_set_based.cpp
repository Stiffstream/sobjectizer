/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A flat-set based storage for agent's subscriptions information.
 *
 * \since v.5.8.2
 */

#include <so_5/impl/subscription_storage_iface.hpp>

#include <so_5/details/rollback_on_exception.hpp>

#include <algorithm>
#include <iterator>
#include <tuple>
#include <vector>

namespace so_5
{

namespace impl
{

/*!
 * \brief A flat-set storage for agent's subscriptions information.
 *
 * \since v.5.8.2
 *
 */
namespace flat_set_based_subscr_storage
{

/*!
 * \brief A flat-set based storage for agent's subscriptions information.
 *
 * This implementation uses a sorted vector (aka "flat_set") of subscr_info_t.
 * Binary search is used for searching subscription, create and drop subscription
 * operations.
 *
 * An std::vector is used as underlying storage. It will grow as necessary. Initial
 * capacity is specified in the constructor.
 *
 * \since v.5.8.2
 */
class storage_t : public subscription_storage_t
	{
	public :
		storage_t(
			std::size_t initial_capacity );
		~storage_t() override;

		virtual void
		create_event_subscription(
			const mbox_t & mbox_ref,
			const std::type_index & type_index,
			abstract_message_sink_t & message_sink,
			const state_t & target_state,
			const event_handler_method_t & method,
			thread_safety_t thread_safety,
			event_handler_kind_t handler_kind ) override;

		virtual void
		drop_subscription(
			const mbox_t & mbox,
			const std::type_index & msg_type,
			const state_t & target_state ) noexcept override;

		void
		drop_subscription_for_all_states(
			const mbox_t & mbox,
			const std::type_index & msg_type ) noexcept override;

		void
		drop_all_subscriptions() noexcept override;

		const event_handler_data_t *
		find_handler(
			mbox_id_t mbox_id,
			const std::type_index & msg_type,
			const state_t & current_state ) const noexcept override;

		void
		debug_dump( std::ostream & to ) const override;

		void
		drop_content() noexcept override;

		subscription_storage_common::subscr_info_vector_t
		query_content() const override;

		void
		setup_content(
			subscription_storage_common::subscr_info_vector_t && info ) override;

		std::size_t
		query_subscriptions_count() const override;

	private :
		using info_t = subscription_storage_common::subscr_info_t;
		using subscr_info_vector_t =
				subscription_storage_common::subscr_info_vector_t;

		//! A helper predicate for searching the same mbox and message type pairs.
		/*!
		 * This predicate is useful when the pointer to target state has be
		 * be ignored. For example, when we have to remove all subscriptions
		 * for all states.
		 */
		struct is_same_mbox_msg_t
			{
				const mbox_id_t m_id;
				const std::type_index & m_type;

				[[nodiscard]] bool
				operator()( const info_t & info ) const noexcept
					{
						return m_id == info.m_mbox->id() &&
								m_type == info.m_msg_type;
					}
			};

		/*!
		 * @brief Helper type for storing only key information about a subscription.
		 *
		 * For fast search in a vector of subscriptions we have to deal with
		 * only a few of key fields of subscr_info_t. This helper type allows to
		 * agregate all those fields in a (rather) small object.
		 */
		struct key_info_t
			{
				mbox_id_t m_mbox_id;
				const std::type_index & m_msg_type;
				const state_t * m_state;
			};

		//! A helper predicate for searching specific subscription
		/*!
		 * This predicate is intended to be used for adding new subscription
		 * and removing of an existing subscription.
		 *
		 * It has several operator() to be used for comparison if different
		 * applications.
		 */
		struct key_info_comparator_t
			{
				[[nodiscard]] bool
				operator()( const key_info_t & a, const key_info_t & b ) const noexcept
					{
						if( a.m_mbox_id < b.m_mbox_id )
							return true;
						else if( a.m_mbox_id == b.m_mbox_id )
							{
								if( a.m_msg_type < b.m_msg_type )
									return true;
								else if( a.m_msg_type == b.m_msg_type )
									{
										// NOTE: it's UB to compare two arbitrary pointers.
										using ptr_comparator_t = std::less< const state_t * >;
										return ptr_comparator_t{}( a.m_state, b.m_state );
									}
							}

						return false;
					}

				[[nodiscard]] bool
				operator()( const info_t & a, const key_info_t & b ) const noexcept
					{
						return (*this)(
								key_info_t{ a.m_mbox->id(), a.m_msg_type, a.m_state },
								b );
					}

				[[nodiscard]] bool
				operator()( const info_t & a, const info_t & b ) const noexcept
					{
						return (*this)(
								key_info_t{ a.m_mbox->id(), a.m_msg_type, a.m_state },
								key_info_t{ b.m_mbox->id(), b.m_msg_type, b.m_state } );
					}
			};

		//! Subscription information.
		subscr_info_vector_t m_events;

		void
		destroy_all_subscriptions() noexcept;

		/*!
		 * @brief Helper for checking presence of subscriptions for the same
		 * message from the same mbox.
		 *
		 * Accepts a valid iterator and returns true if there are at least one
		 * item around it (one to the left or/and one to the right) that
		 * has the same mbox_id and msg_type.
		 *
		 * This helper handles case when @a it points to the first or
		 * the right item of m_events.
		 *
		 * @attention
		 * The @a it should not be "past the end" iterator.
		 */
		[[nodiscard]] bool
		check_presence_of_mbox_msg_type_info_around_it(
			subscr_info_vector_t::iterator it,
			const is_same_mbox_msg_t & predicate ) const noexcept;
	};

namespace
{

/*!
 * @brief Helper to check if two objects are for the same subscription.
 *
 * @note
 * Only mbox_id, msg_type and target_state are compared.
 */
[[nodiscard]] bool
is_equal(
	const subscription_storage_common::subscr_info_t & a,
	const subscription_storage_common::subscr_info_t & b ) noexcept
{
	return a.m_mbox->id() == b.m_mbox->id()
			&& a.m_msg_type == b.m_msg_type
			&& a.m_state == b.m_state
			;
}

/*!
 * @brief Helper to check if subscription information the same.
 *
 * @note
 * Only mbox_id, msg_type and target_state are compared.
 */
[[nodiscard]] bool
is_equal(
	const subscription_storage_common::subscr_info_t & a,
	mbox_id_t mbox_id,
	const std::type_index & msg_type,
	const state_t * target_state ) noexcept
{
	return a.m_mbox->id() == mbox_id
			&& a.m_msg_type == msg_type
			&& a.m_state == target_state
			;
}

} /* namespace anonymous */

storage_t::storage_t(
	std::size_t initial_capacity )
	{
		m_events.reserve( initial_capacity );
	}

storage_t::~storage_t()
	{
		destroy_all_subscriptions();
	}

void
storage_t::create_event_subscription(
	const mbox_t & mbox,
	const std::type_index & msg_type,
	abstract_message_sink_t & message_sink,
	const state_t & target_state,
	const event_handler_method_t & method,
	thread_safety_t thread_safety,
	event_handler_kind_t handler_kind )
	{
		using namespace std;
		using namespace subscription_storage_common;

		info_t info_to_store{
				mbox,
				msg_type,
				message_sink,
				target_state,
				method,
				thread_safety,
				handler_kind
			};

		// Check that this subscription is new.
		auto it = std::lower_bound(
				m_events.begin(), m_events.end(),
				info_to_store,
				key_info_comparator_t{} );
		if( it != m_events.end() && is_equal( *it, info_to_store ) )
			// Subscription already exists. It's an error!
			SO_5_THROW_EXCEPTION(
				rc_evt_handler_already_provided,
				"agent is already subscribed to message, " +
				make_subscription_description( mbox, msg_type, target_state ) );

		// Just add subscription to the storage.
		it = m_events.insert( it, std::move(info_to_store) );

		// Need to check is there existing information for (mbox_id, msg_type)
		// pair. If it's already here then there is no need to call
		// subscribe_event_handler for the mbox.
		const bool info_for_mbox_msg_type_exists =
				check_presence_of_mbox_msg_type_info_around_it(
						it,
						is_same_mbox_msg_t{ mbox->id(), msg_type } );

		// Note: since v.5.5.9 mbox subscription is initiated even if
		// it is MPSC mboxes. It is important for the case of message
		// delivery tracing.

		if( !info_for_mbox_msg_type_exists )
			{
				// Mbox must create subscription.
				so_5::details::do_with_rollback_on_exception(
					[&] {
						mbox->subscribe_event_handler(
								msg_type,
								message_sink );
					},
					[&] {
						m_events.erase( it );
					} );
			}
	}

void
storage_t::drop_subscription(
	const mbox_t & mbox,
	const std::type_index & msg_type,
	const state_t & target_state ) noexcept
	{
		using namespace std;

		auto existed_position = std::lower_bound(
				m_events.begin(), m_events.end(),
				key_info_t{ mbox->id(), msg_type, std::addressof(target_state) },
				key_info_comparator_t{} );
		if( existed_position != m_events.end()
				&& is_equal( *existed_position,
						mbox->id(), msg_type, std::addressof(target_state) ) )
			{
				// This value may be necessary for unsubscription.
				abstract_message_sink_t & message_sink =
						existed_position->m_message_sink.get();

				// Need to check is there existing information for (mbox_id, msg_type)
				// pair. If it's already here then there is no need to call
				// subscribe_event_handler for the mbox.
				const bool info_for_mbox_msg_type_exists =
						check_presence_of_mbox_msg_type_info_around_it(
								existed_position,
								is_same_mbox_msg_t{ mbox->id(), msg_type } );

				// Item is no more needed.
				m_events.erase( existed_position );

				// Note v.5.5.9 unsubscribe_event_handler is called for
				// mbox even if it is MPSC mbox. It is necessary for the case
				// of message delivery tracing.

				// If there is no more subscriptions to that mbox then
				// the mbox must remove information about that agent.
				if( !info_for_mbox_msg_type_exists )
					{
						// If we are here then there is no more references
						// to the mbox. And mbox must not hold reference
						// to the agent.
						mbox->unsubscribe_event_handler( msg_type, message_sink );
					}
			}
	}

void
storage_t::drop_subscription_for_all_states(
	const mbox_t & mbox,
	const std::type_index & msg_type ) noexcept
	{
		using namespace std;

		const auto predicate = is_same_mbox_msg_t{ mbox->id(), msg_type };
		if( auto it = std::lower_bound( m_events.begin(), m_events.end(),
					// NOTE: use NULL instead of actual pointer to a state.
					key_info_t{ mbox->id(), msg_type, nullptr },
					key_info_comparator_t{} );
				it != m_events.end() && predicate( *it ) )
			{
				// There are subscriptions to be removed.
				// Have to store message_sink reference because it has to
				// be passed to unsubscribe_event_handler.
				auto & message_sink = it->m_message_sink.get();

				// Remove all items that match the predicate.
				m_events.erase(
						remove_if( it, end( m_events ), predicate ),
						end( m_events ) );

				// Note: since v.5.5.9 mbox unsubscription is initiated even if
				// it is MPSC mboxes. It is important for the case of message
				// delivery tracing.
				mbox->unsubscribe_event_handler( msg_type, message_sink );
			}
	}

void
storage_t::drop_all_subscriptions() noexcept
	{
		destroy_all_subscriptions();
	}

const event_handler_data_t *
storage_t::find_handler(
	mbox_id_t mbox_id,
	const std::type_index & msg_type,
	const state_t & current_state ) const noexcept
	{
		auto existed_position = std::lower_bound(
				m_events.begin(), m_events.end(),
				key_info_t{ mbox_id, msg_type, std::addressof(current_state) },
				key_info_comparator_t{} );
		if( existed_position != m_events.end()
				&& is_equal( *existed_position,
						mbox_id, msg_type, std::addressof(current_state) ) )
		{
			return std::addressof(existed_position->m_handler);
		}
		else
			return nullptr;
	}

void
storage_t::debug_dump( std::ostream & to ) const
	{
		for( const auto & e : m_events )
			to << "{" << e.m_mbox->id() << ", "
					<< e.m_msg_type.name() << ", "
					<< e.m_state->query_name() << "}"
					<< std::endl;
	}

void
storage_t::destroy_all_subscriptions() noexcept
	{
		if( m_events.empty() )
			// Nothing to do at empty subscription list.
			return;

		using namespace std;

		// Destroy all subscriptions for unique (mbox, msg_type).
		const auto total_items = m_events.size();
		for( std::size_t i = 0u; i < total_items; )
			{
				auto & current_info = m_events[ i ];
				current_info.m_mbox->unsubscribe_event_handler(
						current_info.m_msg_type,
						current_info.m_message_sink );

				// We should skip all consequtive items with the same
				// (mbox, msg_type) pairs.
				std::size_t j = 1u;
				for( ; (i+j) < total_items; ++j )
					{
						const auto & next_info = m_events[ i+j ];
						if( current_info.m_mbox->id() != next_info.m_mbox->id() ||
								current_info.m_msg_type != next_info.m_msg_type )
							break;
					}

				i += j;
			}

		// Cleanup subscription vector.
		drop_content();
	}

bool
storage_t::check_presence_of_mbox_msg_type_info_around_it(
	subscr_info_vector_t::iterator it,
	const is_same_mbox_msg_t & predicate ) const noexcept
	{
		bool result = false;
		if( it != m_events.begin() )
			result = predicate( *(std::prev(it)) );

		if( !result )
		{
			if( auto next = std::next(it); next != m_events.end() )
				result = predicate( *(std::next(it)) );
		}

		return result;
	}

void
storage_t::drop_content() noexcept
	{
		m_events.clear();
	}

subscription_storage_common::subscr_info_vector_t
storage_t::query_content() const
	{
		return m_events;
	}

void
storage_t::setup_content(
	subscription_storage_common::subscr_info_vector_t && info )
	{
		m_events = std::move( info );
		std::sort( m_events.begin(), m_events.end(), key_info_comparator_t{} );
	}

std::size_t
storage_t::query_subscriptions_count() const
	{
		return m_events.size();
	}

} /* namespace flat_set_based_subscr_storage */

} /* namespace impl */

SO_5_FUNC subscription_storage_factory_t
flat_set_based_subscription_storage_factory(
	std::size_t initial_capacity )
	{
		return [initial_capacity]() {
			return impl::subscription_storage_unique_ptr_t(
					new impl::flat_set_based_subscr_storage::storage_t(
							initial_capacity ) );
		};
	}

} /* namespace so_5 */

