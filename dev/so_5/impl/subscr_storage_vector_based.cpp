/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.3
 *
 * \file
 * \brief A vector-based storage for agent's subscriptions information.
 */

#include <so_5/impl/subscription_storage_iface.hpp>

#include <algorithm>
#include <vector>
#include <iterator>

#include <so_5/details/rollback_on_exception.hpp>

namespace so_5
{

namespace impl
{

/*!
 * \since
 * v.5.5.3
 *
 * \brief A vector-based storage for agent's subscriptions information.
 */
namespace vector_based_subscr_storage
{

/*!
 * \since
 * v.5.5.3
 *
 * \brief A vector-based storage for agent's subscriptions information.
 *
 * This is very simple implementation of subscription storage which
 * uses std::vector for storing information.
 *
 * All manipulation is performed by very simple linear search inside
 * that vector. For agents with few subscriptions this will be the most
 * efficient approach.
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

		//! A helper predicate for searching the same
		//! mbox and message type pairs.
		struct is_same_mbox_msg
			{
				const mbox_id_t m_id;
				const std::type_index & m_type;

				bool
				operator()( const info_t & info ) const
					{
						return m_id == info.m_mbox->id() &&
								m_type == info.m_msg_type;
					}
			};

		//! Subscription information.
		subscr_info_vector_t m_events;

		void
		destroy_all_subscriptions() noexcept;
	};

namespace
{
	template< class Container >
	auto
	find( Container & c,
		const mbox_id_t & mbox_id,
		const std::type_index & msg_type,
		const state_t & target_state ) -> decltype( c.begin() )
		{
			using namespace std;

			return find_if( begin( c ), end( c ),
				[&]( typename Container::value_type const & o ) {
					return ( o.m_mbox->id() == mbox_id &&
						o.m_msg_type == msg_type &&
						o.m_state == &target_state );
				} );
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

		const auto mbox_id = mbox->id();

		// Check that this subscription is new.
		auto existed_position = find(
				m_events, mbox_id, msg_type, target_state );

		if( existed_position != m_events.end() )
			SO_5_THROW_EXCEPTION(
				rc_evt_handler_already_provided,
				"agent is already subscribed to message, " +
				make_subscription_description( mbox, msg_type, target_state ) );

		// Just add subscription to the end.
		m_events.emplace_back(
				mbox,
				msg_type,
				message_sink,
				target_state,
				method,
				thread_safety,
				handler_kind );

		// Note: since v.5.5.9 mbox subscription is initiated even if
		// it is MPSC mboxes. It is important for the case of message
		// delivery tracing.

		// If there is no subscription for that mbox it must be created.
		// Last item in m_events should not be checked becase it is
		// description of the just added subscription.
		auto last_to_check = --end( m_events );
		if( last_to_check == find_if(
				begin( m_events ), last_to_check,
				is_same_mbox_msg{ mbox_id, msg_type } ) )
			{
				// Mbox must create subscription.
				so_5::details::do_with_rollback_on_exception(
					[&] {
						mbox->subscribe_event_handler(
								msg_type,
								message_sink );
					},
					[&] {
						m_events.pop_back();
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

		const auto mbox_id = mbox->id();

		auto existed_position = find(
				m_events, mbox_id, msg_type, target_state );
		if( existed_position != m_events.end() )
			{
				// This value may be necessary for unsubscription.
				abstract_message_sink_t & message_sink = existed_position->m_message_sink.get();

				// Item is no more needed.
				m_events.erase( existed_position );

				// Note v.5.5.9 unsubscribe_event_handler is called for
				// mbox even if it is MPSC mbox. It is necessary for the case
				// of message delivery tracing.

				// If there is no more subscriptions to that mbox then
				// the mbox must remove information about that agent.
				if( end( m_events ) == find_if(
						begin( m_events ), end( m_events ),
						is_same_mbox_msg{ mbox_id, msg_type } ) )
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

		const auto predicate = is_same_mbox_msg{ mbox->id(), msg_type };
		if( auto it =
				find_if( begin( m_events ), end( m_events ), predicate );
				it != end( m_events ) )
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
		auto it = find( m_events, mbox_id, msg_type, current_state );

		if( it != std::end( m_events ) )
			return &(it->m_handler);
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

		// Step one.
		//
		// Sort all event_info to have all subscriptions for the
		// same (mbox, msg_type) one after another.
		sort( begin( m_events ), end( m_events ),
				[]( const auto & a, const auto & b )
				{
					return a.m_mbox->id() < b.m_mbox->id() ||
							( a.m_mbox->id() == b.m_mbox->id() &&
							 a.m_msg_type < b.m_msg_type );
				} );

		// Step two.
		//
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

		// Third step.
		//
		// Cleanup subscription vector.
		drop_content();
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
	}

std::size_t
storage_t::query_subscriptions_count() const
	{
		return m_events.size();
	}

} /* namespace vector_based_subscr_storage */

} /* namespace impl */

SO_5_FUNC subscription_storage_factory_t
vector_based_subscription_storage_factory(
	std::size_t initial_capacity )
	{
		return [initial_capacity]() {
			return impl::subscription_storage_unique_ptr_t(
					new impl::vector_based_subscr_storage::storage_t(
							initial_capacity ) );
		};
	}

} /* namespace so_5 */

