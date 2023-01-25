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

#include <so_5/message_limit.hpp>

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

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
 * v.5.5.4, v.5.7.1
 *
 * \brief An interface for storage of messages limits for one agent.
 *
 * \note
 * It's an interface since v.5.7.1. In previous versions of SObjectizer
 * it was a class.
 */
class info_storage_t
	{
		info_storage_t( const info_storage_t & ) = delete;
		info_storage_t & operator=( const info_storage_t & ) = delete;

	public :
		info_storage_t() = default;
		virtual ~info_storage_t() = default;

		[[nodiscard]]
		virtual const control_block_t *
		find( const std::type_index & msg_type ) const = 0;

		[[nodiscard]]
		virtual const control_block_t *
		find_or_create( const std::type_index & msg_type ) = 0;
	};

//
// fixed_info_storage_t
//
/*!
 * \since
 * v.5.5.4, v.5.7.1
 *
 * \brief A fixed-capacity storage for message limits for one agent.
 */
class fixed_info_storage_t final : public info_storage_t
	{
	public :
		//! Initializing constructor.
		fixed_info_storage_t(
			//! Source description of limits.
			//! Since v.5.7.1 this container is expected to be
			//! sorted and checked for duplicates.
			description_container_t && descriptions )
			:	m_blocks( build_blocks( std::move( descriptions ) ) )
			,	m_small_container( m_blocks.size() <= 8 )
			{}

		[[nodiscard]]
		const control_block_t *
		find( const std::type_index & msg_type ) const override
			{
				auto r = find_block( msg_type );

				if( r )
					return &(r->m_control_block);

				return nullptr;
			}

		[[nodiscard]]
		const control_block_t *
		find_or_create( const std::type_index & msg_type ) override
			{
				return find( msg_type );
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

//
// growable_info_storage_t
//
/*!
 * \since
 * v.5.7.1
 *
 * \brief A storage of growable capacity for message limits for one agent.
 */
class growable_info_storage_t final : public info_storage_t
	{
	public :
		//! Initializing constructor.
		growable_info_storage_t(
			//! Description of the default limit.
			description_t && default_limit_description,
			//! Source description of limits.
			description_container_t && descriptions )
			:	m_default_limit_description{ std::move(default_limit_description) }
			,	m_blocks{ build_blocks( std::move(descriptions) ) }
			{}

		[[nodiscard]]
		const control_block_t *
		find( const std::type_index & msg_type ) const override
			{
				auto it = m_blocks.find( msg_type );
				if( it == m_blocks.end() )
					return nullptr;

				return std::addressof( it->second );
			}

		[[nodiscard]]
		const control_block_t *
		find_or_create( const std::type_index & msg_type ) override
			{
				auto it = m_blocks.find( msg_type );
				if( it == m_blocks.end() )
					{
						// A new control block for unknown message type
						// should be created.
						it = m_blocks.emplace( msg_type,
								control_block_t{
										m_default_limit_description.m_limit,
										m_default_limit_description.m_action
								} ).first;
					}

				return std::addressof( it->second );
			}

	private :
		//! Type of storage for control_blocks.
		using blocks_storage_t = std::map< std::type_index, control_block_t >;

		//! Description of the default limit.
		const description_t m_default_limit_description;

		//! Storage of control_blocks.
		blocks_storage_t m_blocks;

		//! Run-time limit information builder.
		[[nodiscard]]
		static blocks_storage_t
		build_blocks( description_container_t && descriptions )
			{
				blocks_storage_t result;

				for( auto & d : descriptions )
					result.emplace( d.m_msg_type,
							control_block_t{ d.m_limit, std::move(d.m_action) } );

				return result;
			}
	};

namespace description_preparation_details
{

/*!
 *
 * Returns sorted array as the first item of the result tuple.
 *
 * If there is a description for any_unspecified_message type then
 * this description is removed from \a original_descriptions and
 * it is returned as the second item of the result tuple.
 *
 * Throws if there is a duplicate in the \a original_descriptions.
 *
 * \since
 * v.5.7.1
 */
[[nodiscard]]
inline std::tuple< description_container_t, std::optional<description_t> >
prepare(
	description_container_t original_descriptions )
{
	using namespace std;

	optional<description_t> default_limit;

	// Descriptions must be sorted.
	sort( begin( original_descriptions ), end( original_descriptions ),
			[]( auto & a, auto & b ) {
				return a.m_msg_type < b.m_msg_type;
			} );

	// There must not be duplicates.
	auto duplicate = adjacent_find(
			begin( original_descriptions ), end( original_descriptions ),
			[]( auto & a, auto & b ) {
				return a.m_msg_type == b.m_msg_type;
			} );
	if( duplicate != end( original_descriptions ) )
		SO_5_THROW_EXCEPTION( rc_several_limits_for_one_message_type,
				std::string( "several limits are defined for message; "
						"msg_type: " ) + duplicate->m_msg_type.name() );

	// Try to find a description for special `any_unspecified_message` mark.
	const std::type_index mark_type{ typeid(any_unspecified_message) };
	auto mark_it = find_if(
			begin( original_descriptions ), end( original_descriptions ),
			[mark_type]( const auto & d ) {
				return d.m_msg_type == mark_type;
			} );
	if( mark_it != end( original_descriptions ) )
		{
			default_limit = std::move(*mark_it);
			original_descriptions.erase( mark_it );
		}

	return { std::move(original_descriptions), std::move(default_limit) };
}

} /* namespace description_preparation_details */

//! Create info_storage object if there are some message limits.
[[nodiscard]]
inline static std::unique_ptr< info_storage_t >
create_info_storage_if_necessary(
	description_container_t && descriptions )
	{
		std::unique_ptr< info_storage_t > result;

		if( !descriptions.empty() )
			{
				auto [sorted_descs, default_limit] =
						description_preparation_details::prepare(
								std::move(descriptions) );

				if( !default_limit )
					// There is no default limit, so fixed_info_storage_t
					// should be used.
					result = std::make_unique< fixed_info_storage_t >(
							std::move(sorted_descs) );
				else
					result = std::make_unique< growable_info_storage_t >(
							std::move(*default_limit),
							std::move(sorted_descs) );
			}

		return result;
	}

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
try_to_deliver_to_consumer(
	//! Mbox that is used for message delivery.
	/*!
	 * Added in v.5.5.23 for support of enveloped messages.
	 */
	mbox_id_t mbox_id,
	//! Receiver of the message or service request.
	const message_sink_t & receiver,
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

