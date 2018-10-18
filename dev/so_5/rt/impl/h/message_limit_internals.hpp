/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \brief Private part of message limit implementation.
 */

#pragma once

#include <so_5/rt/h/message_limit.hpp>

#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>

namespace so_5
{

namespace message_limit
{


namespace impl
{

//
// info_block_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Run-time data for limit of one message type.
 */
struct info_block_t
	{
		//! Type of the message.
		std::type_index m_msg_type;

		//! Run-time data for the message type.
		control_block_t m_control_block;

		//! Initializing constructor.
		info_block_t(
			//! Type of the message.
			std::type_index msg_type,
			//! Limit for that message type.
			unsigned int limit,
			//! Reaction to the limit overflow.
			action_t action )
			:	m_msg_type( std::move( msg_type ) )
			,	m_control_block( limit, std::move( action ) )
			{}
	};

//
// info_block_container_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Type of container for limits' run-time data.
 */
using info_block_container_t = std::vector< info_block_t >;

//
// info_storage_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief A storage for message limits for one agent.
 */
class info_storage_t
	{
		info_storage_t( const info_storage_t & ) = delete;
		info_storage_t & operator=( const info_storage_t & ) = delete;

	public :
		//! Initializing constructor.
		info_storage_t(
			//! Source description of limits.
			description_container_t && descriptions )
			:	m_blocks( build_blocks( std::move( descriptions ) ) )
			,	m_small_container( m_blocks.size() <= 8 )
			{}

		inline const control_block_t *
		find( const std::type_index & msg_type ) const
			{
				auto r = find_block( msg_type );

				if( r )
					return &(r->m_control_block);

				return nullptr;
			}

		//! Create info_storage object if there are some message limits.
		inline static std::unique_ptr< info_storage_t >
		create_if_necessary(
			description_container_t && descriptions )
			{
				std::unique_ptr< info_storage_t > result;

				if( !descriptions.empty() )
					result.reset(
							new info_storage_t( std::move( descriptions ) ) );

				return result;
			}

	private :
		//! Information about limits.
		const info_block_container_t m_blocks;

		//! Is the container is small and linear search must be used?
		const bool m_small_container;

		//! Run-time limit information builder.
		inline static info_block_container_t
		build_blocks( description_container_t && descriptions )
			{
				using namespace std;

				info_block_container_t result;
				result.reserve( descriptions.size() );

				transform( begin( descriptions ), end( descriptions ),
						back_inserter( result ),
						[]( description_t & d ) {
							return info_block_t{
									d.m_msg_type,
									d.m_limit,
									std::move( d.m_action )
								};
						} );

				// Result must be sorted.
				sort( begin( result ), end( result ),
						[]( const info_block_t & a, const info_block_t & b ) {
							return a.m_msg_type < b.m_msg_type;
						} );

				// There must not be duplicates.
				auto duplicate = adjacent_find( begin( result ), end( result ),
						[]( const info_block_t & a, const info_block_t & b ) {
							return a.m_msg_type == b.m_msg_type;
						} );
				if( duplicate != end( result ) )
					SO_5_THROW_EXCEPTION( rc_several_limits_for_one_message_type,
							std::string( "several limits are defined for message; "
									"msg_type: " ) + duplicate->m_msg_type.name() );

				return result;
			}

		//! Search for info_block.
		inline const info_block_t *
		find_block( const std::type_index & msg_type ) const
			{
				if( m_small_container )
					return find_block_in_small_container( msg_type );
				else
					return find_block_in_large_container( msg_type );
			}


		//! Search for info_block in the small container.
		inline const info_block_t *
		find_block_in_small_container(
			const std::type_index & msg_type ) const
			{
				using namespace std;

				// Simple linear search because it is more effective
				// on a small containers.
				auto r = find_if( begin( m_blocks ), end( m_blocks ),
						[&]( const info_block_t & blk ) {
							return blk.m_msg_type == msg_type;
						} );
				if( r != end( m_blocks ) )
					return &(*r);

				return nullptr;
			}

		//! Search for info_block in the large container.
		inline const info_block_t *
		find_block_in_large_container(
			const std::type_index & msg_type ) const
			{
				using namespace std;

				// Use binary search.
				auto left = begin( m_blocks );
				auto count = distance( left, end( m_blocks ) );

				while( count > 0 )
					{
						auto step = count / 2;
						auto middle = left + step;
						if( middle->m_msg_type == msg_type )
							return &(*middle);
						else if( middle->m_msg_type < msg_type )
							{
								left = middle + 1;
								count -= step + 1;
							}
						else
							count = step;
					}

				return nullptr;
			}
	};

namespace
{

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunneeded-member-function"
#pragma clang diagnostic ignored "-Wunused-member-function"
#endif

/*!
 * \since
 * v.5.5.4
 *
 * \brief Helper class to rollback message count change in
 * case of an exception.
 */
struct decrement_on_exception_t
{
	const control_block_t * m_limit;
	bool m_commited = false;

	decrement_on_exception_t( const control_block_t * limit )
		:	m_limit( limit )
	{}
	~decrement_on_exception_t()
	{
		if( !m_commited )
			--(m_limit->m_count);
	}

	void
	commit() { m_commited = true; }
};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} /* namespace anonymous */

/*!
 * \since
 * v.5.5.4
 *
 * \brief A helper function for pushing a message or a service
 * request to agent with respect to message limit.
 *
 * \tparam Lambda lambda-function to do actual pushing.
 */
template< typename Lambda >
void
try_to_deliver_to_agent(
	//! Mbox that is used for message delivery.
	/*!
	 * Added in v.5.5.23 for support of enveloped messages.
	 */
	mbox_id_t mbox_id,
	//! It is a message or service request?
	invocation_type_t invocation_type,
	//! Receiver of the message or service request.
	const agent_t & receiver,
	//! Optional message limit.
	//! Value nullptr means that there is no message limit to control.
	const control_block_t * limit,
	//! Type of message to be delivered.
	const std::type_index & msg_type,
	//! Message instance to be delivered.
	const message_ref_t & what_to_deliver,
	//! Deep of overlimit reactions recursion.
	unsigned int overlimit_reaction_deep,
	//! Message delivery tracer to be used inside overlimit reaction.
	const message_limit::impl::action_msg_tracer_t * tracer,
	//! Actual delivery action.
	Lambda delivery_action )
{
	if( limit && ( limit->m_limit < ++(limit->m_count) ) )
	{
		--(limit->m_count);

		limit->m_action(
			overlimit_context_t{
				mbox_id,
				receiver,
				*limit,
				invocation_type,
				overlimit_reaction_deep,
				msg_type,
				what_to_deliver,
				tracer } );
	}
	else
	{
		decrement_on_exception_t exception_guard( limit );

		delivery_action();

		exception_guard.commit();
	}
}

} /* namespace impl */

} /* namespace message_limit */

} /* namespace so_5 */

