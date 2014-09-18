/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief A storage for agent's subscriptions information.
 */

#include <so_5/rt/impl/h/subscription_storage.hpp>

#include <sstream>

namespace so_5
{

namespace rt
{

namespace impl
{

using namespace subscription_details;

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

	std::string
	make_subscription_description(
		const mbox_ref_t & mbox_ref,
		std::type_index msg_type,
		const state_t & state )
	{
		std::ostringstream s;
		s << "(mbox:'" << mbox_ref->query_name()
			<< "', msg_type:'" << msg_type.name() << "', state:'"
			<< state.query_name() << "')";

		return s.str();
	}

} /* namespace anonymous */

/*!
 * \since v.5.4.0
 * \brief A storage for agent's subscriptions information.
 */
subscription_storage_t::subscription_storage_t( agent_t * owner )
	:	m_owner( owner )
	{}

subscription_storage_t::~subscription_storage_t()
	{
		destroy_all_subscriptions();
	}

void
subscription_storage_t::create_event_subscription(
	const mbox_ref_t & mbox_ref,
	std::type_index type_index,
	const state_t & target_state,
	const event_handler_method_t & method,
	thread_safety_t thread_safety )
	{
		key_t key( mbox_ref->id(), type_index, target_state );

		auto insertion_result = m_map.emplace( key, mbox_ref );

		if( !insertion_result.second )
			SO_5_THROW_EXCEPTION(
				rc_evt_handler_already_provided,
				"agent is already subscribed to message, " +
				make_subscription_description( mbox_ref, type_index, target_state ) );

		try
			{
				m_hash_table.emplace( &(insertion_result.first->first),
						event_handler_data_t( method, thread_safety ) );
			}
		catch( ... )
			{
				// Rollback agent's subscription.
				m_map.erase( insertion_result.first );
				throw;
			}

		auto mbox_msg_known = is_known_mbox_msg_pair(
				m_map, insertion_result.first );
		if( !mbox_msg_known )
		{
			// Mbox must create subscription.
			try
			{
				mbox_ref->subscribe_event_handler( type_index, m_owner );
			}
			catch( ... )
			{
				// Rollback agent's subscription.
				m_hash_table.erase( &(insertion_result.first->first) );
				m_map.erase( insertion_result.first );
				throw;
			}
		}
	}

void
subscription_storage_t::drop_subscription(
	const std::type_index & type_index,
	const mbox_ref_t & mbox_ref,
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
				mbox_ref->unsubscribe_event_handlers(
					type_index,
					m_owner );
			}
		}
	}

void
subscription_storage_t::drop_subscription_for_all_states(
	const std::type_index & type_index,
	const mbox_ref_t & mbox_ref )
	{
		key_t key( mbox_ref->id(), type_index );

		auto it = m_map.lower_bound( key );
		if( it != m_map.end() )
		{
			while( key.is_same_mbox_msg_pair( it->first ) )
				{
					m_hash_table.erase( &(it->first) );
					m_map.erase( it++ );
				}

			mbox_ref->unsubscribe_event_handlers( type_index, m_owner );
		}
	}

const event_handler_data_t *
subscription_storage_t::find_handler(
	mbox_id_t mbox_id,
	const std::type_index & msg_type,
	const state_t & current_state ) const
	{
		key_t k( mbox_id, msg_type, current_state );
		auto it = m_hash_table.find( &k );
		if( it != m_hash_table.end() )
			return &(it->second);
		else
			return nullptr;
	}

void
subscription_storage_t::debug_dump( std::ostream & to ) const
	{
		for( const auto & v : m_map )
			std::cout << "{" << v.first.m_mbox_id << ", "
					<< v.first.m_msg_type.name() << ", "
					<< v.first.m_state->query_name() << "}"
					<< std::endl;
	}

void
subscription_storage_t::destroy_all_subscriptions()
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
						m_owner );

				previous = &i;
			}
		}

		hash_table_t tmp_hash_table;
		m_hash_table.swap( tmp_hash_table );

		map_t tmp_map;
		m_map.swap( tmp_map );
	}

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

