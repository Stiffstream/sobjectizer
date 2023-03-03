/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.3
 *
 * \file
 * \brief A map-based storage for agent's subscriptions information.
 */

#include <so_5/impl/subscription_storage_iface.hpp>

#include <algorithm>
#include <iterator>
#include <map>

#include <so_5/details/rollback_on_exception.hpp>

namespace so_5
{

namespace impl
{

/*!
 * \since
 * v.5.5.3
 *
 * \brief A map-based storage for agent's subscriptions information.
 */
namespace map_based_subscr_storage
{

/*!
 * \since
 * v.5.5.3
 *
 * \brief A map-based storage for agent's subscriptions information.
 *
 * This is very simple implementation of subscription storage which
 * uses std::map for storing information.
 */
class storage_t : public subscription_storage_t
	{
	public :
		storage_t();
		~storage_t() override;

		void
		create_event_subscription(
			const mbox_t & mbox_ref,
			const std::type_index & type_index,
			abstract_message_sink_t & message_sink,
			const state_t & target_state,
			const event_handler_method_t & method,
			thread_safety_t thread_safety,
			event_handler_kind_t handler_kind ) override;

		void
		drop_subscription(
			const mbox_t & mbox,
			const std::type_index & msg_type,
			const state_t & target_state ) override;

		void
		drop_subscription_for_all_states(
			const mbox_t & mbox,
			const std::type_index & msg_type ) override;

		void
		drop_all_subscriptions() override;

		const event_handler_data_t *
		find_handler(
			mbox_id_t mbox_id,
			const std::type_index & msg_type,
			const state_t & current_state ) const noexcept override;

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
		//! Type of key in subscription's map.
		struct key_t
			{
				mbox_id_t m_mbox_id;
				std::type_index m_msg_type;
				const state_t * m_state;

				key_t(
					mbox_id_t mbox_id,
					std::type_index msg_type,
					const state_t * state )
					:	m_mbox_id( mbox_id )
					,	m_msg_type( std::move( msg_type ) )
					,	m_state( state )
					{}

				bool
				operator<( const key_t & o ) const
					{
						if( m_mbox_id < o.m_mbox_id )
							return true;
						else if( m_mbox_id == o.m_mbox_id )
							{
								if( m_msg_type < o.m_msg_type )
									return true;
								else if( m_msg_type == o.m_msg_type )
									return m_state < o.m_state;
							}

						return false;
					}
			};

		//! Type of value for subscription map's item.
		struct value_t
			{
				//! Reference to mbox.
				/*!
				 * Reference must be stored because we must have
				 * access to mbox during destroyment of all
				 * subscriptions in destructor.
				 */
				const mbox_t m_mbox;

				/*!
				 * Message sink used for that mbox.
				 */
				const std::reference_wrapper< abstract_message_sink_t > m_message_sink;

				/*!
				 * Event handler for that subscription.
				 */
				const event_handler_data_t m_handler;
			};

		//! Type of subscriptions map.
		using subscr_map_t = std::map< key_t, value_t >;

		//! Subscription information.
		subscr_map_t m_events;

		void
		destroy_all_subscriptions();
	};

namespace
{
	template< class C >
	[[nodiscard]]
	auto
	find( C & c,
		const mbox_id_t & mbox_id,
		const std::type_index & msg_type,
		const state_t & target_state ) -> decltype( c.begin() )
		{
			return c.find( typename C::key_type {
					mbox_id, msg_type, &target_state } );
		}

	struct is_same_mbox_msg
		{
			const mbox_id_t m_id;
			const std::type_index & m_type;

			template< class K >
			[[nodiscard]]
			bool
			operator()( const K & k ) const
				{
					return m_id == k.m_mbox_id && m_type == k.m_msg_type;
				}
		};

	template< class M, class IT >
	[[nodiscard]]
	bool
	is_known_mbox_msg_pair( M & s, IT it )
		{
			const is_same_mbox_msg predicate{
					it->first.m_mbox_id, it->first.m_msg_type };

			if( it != s.begin() )
				{
					IT prev = std::prev( it );
					if( predicate( prev->first ) )
						return true;
				}

			IT next = std::next( it );
			if( next != s.end() )
				return predicate( next->first );

			return false;
		}

} /* namespace anonymous */

storage_t::storage_t()
	{}

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
		auto ins_result = m_events.emplace(
					key_t { mbox_id, msg_type, &target_state },
					value_t {
							mbox,
							std::ref( message_sink ),
							event_handler_data_t {
									method,
									thread_safety,
									handler_kind
							}
					}
				);

		// Note: since v.5.5.9 mbox subscription is initiated even if
		// it is MPSC mboxes. It is important for the case of message
		// delivery tracing.

		// If there was no subscription for that mbox
		// then new subscription in the mbox must be created.
		if( !is_known_mbox_msg_pair( m_events, ins_result.first ) )
			{
				so_5::details::do_with_rollback_on_exception(
					[&] {
						mbox->subscribe_event_handler(
								msg_type,
								message_sink );
					},
					[&] {
						m_events.erase( ins_result.first );
					} );
			}
	}

void
storage_t::drop_subscription(
	const mbox_t & mbox,
	const std::type_index & msg_type,
	const state_t & target_state )
	{
		auto existed_position = find(
				m_events, mbox->id(), msg_type, target_state );
		if( existed_position != m_events.end() )
			{
				// Note v.5.5.9 unsubscribe_event_handlers is called for
				// mbox even if it is MPSC mbox. It is necessary for the case
				// of message delivery tracing.

				// We must destroy mbox subscription in case if the agent has no
				// more subscriptions for that mbox+msg_type pair.
				// Detect this while existed_position is not invalidated.
				bool must_unsubscribe_mbox =
						!is_known_mbox_msg_pair( m_events, existed_position );

				// Store a reference to message_sink for a case if
				// unsubscribe_event_handlers has to be called.
				abstract_message_sink_t & sink = existed_position->second.m_message_sink.get();

				m_events.erase( existed_position );

				if( must_unsubscribe_mbox )
					{
						mbox->unsubscribe_event_handlers( msg_type, sink );
					}
			}
	}

void
storage_t::drop_subscription_for_all_states(
	const mbox_t & mbox,
	const std::type_index & msg_type )
	{
		const is_same_mbox_msg is_same{ mbox->id(), msg_type };

		auto lower_bound = m_events.lower_bound(
				key_t{ mbox->id(), msg_type, nullptr } );

		auto need_erase = [&] {
				return lower_bound != std::end(m_events) &&
						is_same( lower_bound->first );
			};
		const bool events_found = need_erase();

		if( events_found )
			{
				// Store a reference to message_sink because it's required
				// for unsubscribe_event_handlers call.
				abstract_message_sink_t & sink = lower_bound->second.m_message_sink.get();

				// Erase all subscribed event handlers.
				do
					{
						m_events.erase( lower_bound++ );
					}
				while( need_erase() );

				// Note: since v.5.5.9 mbox unsubscription is initiated even if
				// it is MPSC mboxes. It is important for the case of message
				// delivery tracing.

				mbox->unsubscribe_event_handlers( msg_type, sink );
			}
	}

void
storage_t::drop_all_subscriptions()
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
			return &(it->second.m_handler);
		else
			return nullptr;
	}

void
storage_t::debug_dump( std::ostream & to ) const
	{
		for( const auto & e : m_events )
			to << "{" << e.first.m_mbox_id << ", "
					<< e.first.m_msg_type.name() << ", "
					<< e.first.m_state->query_name() << "}"
					<< std::endl;
	}

void
storage_t::destroy_all_subscriptions()
	{
		using namespace std;

		auto it = begin( m_events );
		while( it != end( m_events ) )
			{
				auto cur = it++;

				if( it == end( m_events ) || !is_same_mbox_msg{
						cur->first.m_mbox_id,
						cur->first.m_msg_type }( it->first ) )
					{
						cur->second.m_mbox->unsubscribe_event_handlers(
								cur->first.m_msg_type,
								cur->second.m_message_sink.get() );
					}

				m_events.erase( cur );
			}
	}

void
storage_t::drop_content()
	{
		subscr_map_t empty_map;
		m_events.swap( empty_map );
	}

subscription_storage_common::subscr_info_vector_t
storage_t::query_content() const
	{
		using namespace std;
		using namespace subscription_storage_common;

		subscr_info_vector_t result;

		if( !m_events.empty() )
			{
				result.reserve( m_events.size() );

				transform( begin( m_events ), end( m_events ),
						back_inserter( result ),
						[]( const subscr_map_t::value_type & e )
						{
							return subscr_info_t(
									e.second.m_mbox,
									e.first.m_msg_type,
									e.second.m_message_sink.get(),
									*(e.first.m_state),
									e.second.m_handler.m_method,
									e.second.m_handler.m_thread_safety,
									e.second.m_handler.m_kind );
						} );
			}

		return result;
	}

void
storage_t::setup_content(
	subscription_storage_common::subscr_info_vector_t && info )
	{
		using namespace std;
		using namespace subscription_storage_common;

		subscr_map_t events;
		transform( begin(info), end(info),
				inserter( events, events.begin() ),
				[]( const subscr_info_t & i )
				{
					return subscr_map_t::value_type {
							key_t {
								i.m_mbox->id(),
								i.m_msg_type,
								i.m_state
							},
							value_t {
								i.m_mbox,
								i.m_message_sink,
								i.m_handler
							} };
				} );

		m_events.swap( events );
	}

std::size_t
storage_t::query_subscriptions_count() const
	{
		return m_events.size();
	}

} /* namespace map_based_subscr_storage */

} /* namespace impl */

SO_5_FUNC subscription_storage_factory_t
map_based_subscription_storage_factory()
	{
		return []() {
			return impl::subscription_storage_unique_ptr_t(
					new impl::map_based_subscr_storage::storage_t() );
		};
	}

} /* namespace so_5 */

