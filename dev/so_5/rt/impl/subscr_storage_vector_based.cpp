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

#include <so_5/rt/impl/h/subscription_storage_iface.hpp>

#include <algorithm>
#include <vector>
#include <iterator>

#include <so_5/details/h/rollback_on_exception.hpp>

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
			agent_t * owner,
			std::size_t initial_capacity );
		~storage_t() override;

		virtual void
		create_event_subscription(
			const mbox_t & mbox_ref,
			const std::type_index & type_index,
			const message_limit::control_block_t * limit,
			const state_t & target_state,
			const event_handler_method_t & method,
			thread_safety_t thread_safety ) override;

		virtual void
		drop_subscription(
			const mbox_t & mbox,
			const std::type_index & msg_type,
			const state_t & target_state ) override;

		void
		drop_subscription_for_all_states(
			const mbox_t & mbox,
			const std::type_index & msg_type ) override;

		const event_handler_data_t *
		find_handler(
			mbox_id_t mbox_id,
			const std::type_index & msg_type,
			const state_t & current_state ) const SO_5_NOEXCEPT override;

		void
		debug_dump( std::ostream & to ) const override;

		void
		drop_content() override;

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
		destroy_all_subscriptions();
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
	agent_t * owner,
	std::size_t initial_capacity )
	:	subscription_storage_t( owner )
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
	const message_limit::control_block_t * limit,
	const state_t & target_state,
	const event_handler_method_t & method,
	thread_safety_t thread_safety )
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
				mbox, msg_type, target_state, method, thread_safety );

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
								limit,
								owner() );
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
	const state_t & target_state )
	{
		using namespace std;

		const auto mbox_id = mbox->id();

		auto existed_position = find(
				m_events, mbox_id, msg_type, target_state );
		if( existed_position != m_events.end() )
			{
				m_events.erase( existed_position );

				// Note v.5.5.9 unsubscribe_event_handlers is called for
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
						mbox->unsubscribe_event_handlers( msg_type, owner() );
					}
			}
	}

void
storage_t::drop_subscription_for_all_states(
	const mbox_t & mbox,
	const std::type_index & msg_type )
	{
		using namespace std;

		const auto mbox_id = mbox->id();

		const auto old_size = m_events.size();

		m_events.erase(
				remove_if( begin( m_events ), end( m_events ),
						[mbox_id, &msg_type]( const info_t & i ) {
							return i.m_mbox->id() == mbox_id &&
									i.m_msg_type == msg_type;
						} ),
				end( m_events ) );

		// Note: since v.5.5.9 mbox unsubscription is initiated even if
		// it is MPSC mboxes. It is important for the case of message
		// delivery tracing.

		if( old_size != m_events.size() )
			mbox->unsubscribe_event_handlers( msg_type, owner() );
	}

const event_handler_data_t *
storage_t::find_handler(
	mbox_id_t mbox_id,
	const std::type_index & msg_type,
	const state_t & current_state ) const SO_5_NOEXCEPT
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
storage_t::destroy_all_subscriptions()
	{
		if( m_events.empty() )
			// Nothing to do at empty subscription list.
			return;

		using namespace std;

		struct mbox_msg_type_pair_t
			{
				abstract_message_box_t * m_mbox;
				const type_index * m_msg_type;

				bool
				operator<( const mbox_msg_type_pair_t & o ) const
					{
						return m_mbox < o.m_mbox ||
								( m_mbox == o.m_mbox &&
								 (*m_msg_type) < (*o.m_msg_type) );
					}

				bool
				operator==( const mbox_msg_type_pair_t & o ) const
					{
						return m_mbox == o.m_mbox &&
								(*m_msg_type) == (*o.m_msg_type);
					}
			};

		// First step: collect all pointers to mbox-es.
		vector< mbox_msg_type_pair_t > mboxes;
		mboxes.reserve( m_events.size() );

		transform(
				begin( m_events ), end( m_events ),
				back_inserter( mboxes ),
				[]( info_t & i ) {
					return mbox_msg_type_pair_t{ i.m_mbox.get(), &i.m_msg_type };
				} );

		// Second step: remove duplicates.
		sort( begin( mboxes ), end( mboxes ) );
		mboxes.erase(
				unique( begin( mboxes ), end( mboxes ) ),
				end( mboxes ) );

		// Third step: destroy subscription in mboxes.
		for( auto m : mboxes )
			m.m_mbox->unsubscribe_event_handlers( *m.m_msg_type, owner() );

		// Fourth step: cleanup subscription vector.
		drop_content();
	}

void
storage_t::drop_content()
	{
		subscr_info_vector_t empty_events;
		m_events.swap( empty_events );
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
		return [initial_capacity]( agent_t * owner ) {
			return impl::subscription_storage_unique_ptr_t(
					new impl::vector_based_subscr_storage::storage_t(
							owner,
							initial_capacity ) );
		};
	}

} /* namespace so_5 */


