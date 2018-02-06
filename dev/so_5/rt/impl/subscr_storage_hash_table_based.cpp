/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.3
 *
 * \file
 * \brief A hash_table-based storage for agent's subscriptions information.
 */

#include <so_5/rt/impl/h/subscription_storage_iface.hpp>

#include <algorithm>
#include <map>
#include <unordered_map>
#include <iterator>

#include <so_5/details/h/rollback_on_exception.hpp>

namespace so_5
{

namespace impl
{

/*!
 * \since
 * v.5.4.0
 *
 * \brief A hash_table-based storage for agent's subscriptions information.
 */
namespace hash_table_subscr_storage
{

//! Subscription key type.
struct key_t
{
	//! Unique ID of mbox.
	mbox_id_t m_mbox_id;
	//! Message type.
	std::type_index m_msg_type;
	//! State of agent.
	const state_t * m_state;

	//! Default constructor.
	inline key_t()
		:	m_mbox_id( null_mbox_id() )
		,	m_msg_type( typeid(void) )
		,	m_state( nullptr )
		{}

	//! Constructor for the case when it is necessary to
	//! find all keys with (mbox_id, msg_type) prefix.
	inline key_t(
		mbox_id_t mbox_id,
		std::type_index msg_type )
		:	m_mbox_id( mbox_id )
		,	m_msg_type( msg_type )
		,	m_state( nullptr )
		{}

	//! Initializing constructor.
	inline key_t(
		mbox_id_t mbox_id,
		std::type_index msg_type,
		const state_t & state )
		:	m_mbox_id( mbox_id )
		,	m_msg_type( msg_type )
		,	m_state( &state )
		{}

	inline bool
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

	inline bool
	operator==( const key_t & o ) const
		{
			return m_mbox_id == o.m_mbox_id &&
					m_msg_type == o.m_msg_type &&
					m_state == o.m_state;
		}

	inline bool
	is_same_mbox_msg_pair( const key_t & o ) const
		{
			return m_mbox_id == o.m_mbox_id &&
					m_msg_type == o.m_msg_type;
		}
};

//
// hash_t
//
/*!
 * \since
 * v.5.4.0
 *
 * \brief A special class for calculating hash value via pointer to key.
 */
struct hash_t
	{
		typedef const key_t * argument_type;
		typedef std::size_t value_type;

		value_type operator()( argument_type ptr ) const
			{
				// This details have been borrowed from documentation fo
				// boost::hash_combine function:
				// http://www.boost.org/doc/libs/1_46_1/doc/html/hash/reference.html#boost.hash_combine
				//
				const value_type h1 =
					std::hash< so_5::mbox_id_t >()( ptr->m_mbox_id );
				const value_type h2 = h1 ^
					(std::hash< std::type_index >()( ptr->m_msg_type ) +
					 	0x9e3779b9 + (h1 << 6) + (h1 >> 2));

				return h2 ^ (std::hash< const state_t * >()(
							ptr->m_state ) +
						0x9e3779b9 + (h2 << 6) + (h2 >> 2));
			}
	};

//
// equal_to_t
//
/*!
 * \since
 * v.5.4.0
 *
 * \brief A special class for checking equality via pointer to key.
 */
struct equal_to_t
	{
		typedef bool result_type;
		typedef const key_t * first_argument_type;
		typedef const key_t * second_argument_type;

		result_type operator()( first_argument_type a,
			second_argument_type b ) const
			{
				return (*a) == (*b);
			}
	};

namespace
{
	template< class S >
	bool is_known_mbox_msg_pair(
		S & s,
		typename S::iterator it )
	{
		if( it != s.begin() )
		{
			typename S::iterator prev = it;
			--prev;
			if( it->first.is_same_mbox_msg_pair( prev->first ) )
				return true;
		}

		typename S::iterator next = it;
		++next;
		if( next != s.end() )
			return it->first.is_same_mbox_msg_pair( next->first );

		return false;
	}

} /* namespace anonymous */

/*!
 * \since
 * v.5.4.0
 *
 * \brief A storage for agent's subscriptions information.
 *
 * The motivation for this class is the following:
 *
 * Initially an agent had only one subscription map where the key
 * was a compound key: (mbox_id, msg_type, state).
 *
 * This map was used in both operation: subscription creation/removing
 * and event handler searching. But this data structure was very inefficient
 * for event handler lookup. Especially when agent receive the same
 * message from same mbox in different states. It means that for most
 * lookup operations (mbox_id, msg_type) were the same.
 *
 * This class uses std::map and std::unordered_map for different purposes:
 * - std::map is used for subscription management (strictly ordered data
 *   structure makes subscription management a lot easier);
 * - std::unordered_map is used for event handler lookup operations
 *   (hash_table is more effiecient especially for the cases where the
 *   only difference in lookup keys is in state field).
 */
class storage_t : public subscription_storage_t
	{
	public :
		storage_t( agent_t * owner );
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
			const mbox_t & mbox_ref,
			const std::type_index & type_index,
			const state_t & target_state ) override;

		void
		drop_subscription_for_all_states(
			const mbox_t & mbox_ref,
			const std::type_index & type_index ) override;

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
		//! Type of subscription map.
		typedef std::map< key_t, mbox_t > map_t;

		//! Map of subscriptions.
		/*!
		 * It is important to has right order of elements in this map:
		 * all subscription for the same (mbox,message) must be groupped
		 * together.
		 */
		map_t m_map;

		//! Type of event handlers hash table.
		typedef std::unordered_map<
						const key_t *,
						event_handler_data_t,
						hash_t,
						equal_to_t >
				hash_table_t;

		//! Hash table of event handlers.
		hash_table_t m_hash_table;

		void
		destroy_all_subscriptions();
	};

storage_t::storage_t( agent_t * owner )
	:	subscription_storage_t( owner )
	{}

storage_t::~storage_t()
	{
		destroy_all_subscriptions();
	}

void
storage_t::create_event_subscription(
	const mbox_t & mbox_ref,
	const std::type_index & type_index,
	const message_limit::control_block_t * limit,
	const state_t & target_state,
	const event_handler_method_t & method,
	thread_safety_t thread_safety )
	{
		using namespace subscription_storage_common;

		key_t key( mbox_ref->id(), type_index, target_state );

		auto insertion_result = m_map.emplace( key, mbox_ref );

		if( !insertion_result.second )
			SO_5_THROW_EXCEPTION(
				rc_evt_handler_already_provided,
				"agent is already subscribed to message, " +
				make_subscription_description( mbox_ref, type_index, target_state ) );

		so_5::details::do_with_rollback_on_exception(
			[&] {
				m_hash_table.emplace( &(insertion_result.first->first),
						event_handler_data_t( method, thread_safety ) );
			},
			[&] { m_map.erase( insertion_result.first ); } );

		auto mbox_msg_known = is_known_mbox_msg_pair(
				m_map, insertion_result.first );
		if( !mbox_msg_known )
		{
			// Mbox must create subscription.
			so_5::details::do_with_rollback_on_exception(
				[&] {
					mbox_ref->subscribe_event_handler(
							type_index, limit, owner() );
				},
				[&] {
					m_hash_table.erase( &(insertion_result.first->first) );
					m_map.erase( insertion_result.first );
				} );
		}
	}

void
storage_t::drop_subscription(
	const mbox_t & mbox_ref,
	const std::type_index & type_index,
	const state_t & target_state )
	{
		key_t key( mbox_ref->id(), type_index, target_state );

		auto it = m_map.find( key );

		if( m_map.end() != it )
		{
			bool mbox_msg_known = is_known_mbox_msg_pair( m_map, it );

			m_hash_table.erase( &(it->first) );
			m_map.erase( it );

			if( !mbox_msg_known )
			{
				mbox_ref->unsubscribe_event_handlers( type_index, owner() );
			}
		}
	}

void
storage_t::drop_subscription_for_all_states(
	const mbox_t & mbox_ref,
	const std::type_index & type_index )
	{
		const key_t key( mbox_ref->id(), type_index );

		auto it = m_map.lower_bound( key );
		auto need_erase = [&] {
				return it != m_map.end() &&
						key.is_same_mbox_msg_pair( it->first );
			};
		const bool found = need_erase();
		if( found )
		{
			do
				{
					m_hash_table.erase( &(it->first) );
					m_map.erase( it++ );
				}
			while( need_erase() );

			mbox_ref->unsubscribe_event_handlers( type_index, owner() );
		}
	}

const event_handler_data_t *
storage_t::find_handler(
	mbox_id_t mbox_id,
	const std::type_index & msg_type,
	const state_t & current_state ) const SO_5_NOEXCEPT
	{
		key_t k( mbox_id, msg_type, current_state );
		auto it = m_hash_table.find( &k );
		if( it != m_hash_table.end() )
			return &(it->second);
		else
			return nullptr;
	}

void
storage_t::debug_dump( std::ostream & to ) const
	{
		for( const auto & v : m_map )
			to << "{" << v.first.m_mbox_id << ", "
					<< v.first.m_msg_type.name() << ", "
					<< v.first.m_state->query_name() << "}"
					<< std::endl;
	}

void
storage_t::destroy_all_subscriptions()
	{
		{
			const map_t::value_type * previous = nullptr;
			for( auto & i : m_map )
			{
				// Optimisation: for several consequtive keys with
				// the same (mbox, msg_type) pair it is necessary to
				// call unsubscribe_event_handlers only once.
				if( !previous ||
						!previous->first.is_same_mbox_msg_pair( i.first ) )
					i.second->unsubscribe_event_handlers(
						i.first.m_msg_type,
						owner() );

				previous = &i;
			}
		}

		drop_content();
	}

void
storage_t::drop_content()
	{
		hash_table_t tmp_hash_table;
		m_hash_table.swap( tmp_hash_table );

		map_t tmp_map;
		m_map.swap( tmp_map );
	}

subscription_storage_common::subscr_info_vector_t
storage_t::query_content() const
	{
		using namespace std;
		using namespace subscription_storage_common;

		subscr_info_vector_t events;

		if( !m_hash_table.empty() )
			{
				events.reserve( m_hash_table.size() );

				transform( begin(m_hash_table), end(m_hash_table),
						back_inserter(events),
						[this]( const hash_table_t::value_type & i )
						{
							auto map_item = m_map.find( *(i.first) );

							return subscr_info_t {
									map_item->second,
									map_item->first.m_msg_type,
									*(map_item->first.m_state),
									i.second.m_method,
									i.second.m_thread_safety
								};
						} );
			}

		return events;
	}

void
storage_t::setup_content(
	subscription_storage_common::subscr_info_vector_t && info )
	{
		using namespace std;
		using namespace subscription_storage_common;

		map_t fresh_map;
		hash_table_t fresh_table;

		for_each( begin(info), end(info),
			[&]( const subscr_info_t & i )
			{
				key_t k{ i.m_mbox->id(), i.m_msg_type, *(i.m_state) };

				auto ins_result = fresh_map.emplace( k, i.m_mbox );

				fresh_table.emplace( &(ins_result.first->first), i.m_handler );
			} );

		m_map.swap( fresh_map );
		m_hash_table.swap( fresh_table );
	}

std::size_t
storage_t::query_subscriptions_count() const
	{
		return m_hash_table.size();
	}

} /* namespace hash_table_subscr_storage */

} /* namespace impl */

SO_5_FUNC subscription_storage_factory_t
hash_table_based_subscription_storage_factory()
	{
		return []( agent_t * owner ) {
			return impl::subscription_storage_unique_ptr_t(
					new impl::hash_table_subscr_storage::storage_t( owner ) );
		};
	}

} /* namespace so_5 */

