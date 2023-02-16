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

namespace impl
{

//
// message_sink_without_message_limit_t
//
/*!
 * \brief Implementation of message_sink that has no message limits.
 *
 * \since v.5.8.0
 */
class message_sink_without_message_limit_t final
	: public message_sink_t
	{
		//! Owner of the sink.
		agent_t * m_owner;

	public:
		message_sink_without_message_limit_t(
			partially_constructed_agent_ptr_t owner_ptr )
			:	m_owner( owner_ptr.ptr() )
		{}

		environment_t &
		environment() const noexcept override
			{
				return m_owner->so_environment();
			}

		priority_t
		sink_priority() const noexcept override
			{
				return m_owner->so_priority();
			}

		void
		push_event(
			mbox_id_t mbox_id,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int /*overlimit_reaction_deep*/,
			const message_limit::impl::action_msg_tracer_t * /*tracer*/ )
			{
				agent_t::call_push_event(
						*m_owner,
						nullptr /* no message limit */,
						mbox_id,
						msg_type,
						message );
			}
	};

namespace push_event_impl
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
	const so_5::message_limit::control_block_t * m_limit;
	bool m_commited = false;

	decrement_on_exception_t(
		const so_5::message_limit::control_block_t * limit )
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

} /* namespace push_event_impl */

//
// message_sink_with_message_limit_t
//
/*!
 * \brief Implementation of message_sink that handles message limits.
 *
 * \since v.5.8.0
 */
class message_sink_with_message_limit_t final
	: public message_sink_t
	{
		//! Owner of the sink.
		agent_t * m_owner;

		//! Run-time data for the message type.
		so_5::message_limit::control_block_t m_control_block;

	public:
		//! Initializing constructor.
		message_sink_with_message_limit_t(
			//! Owner of the sink.
			partially_constructed_agent_ptr_t owner_ptr,
			//! Limit for that message type.
			unsigned int limit,
			//! Reaction to the limit overflow.
			so_5::message_limit::action_t action )
			:	m_owner( owner_ptr.ptr() )
			,	m_control_block( limit, std::move( action ) )
			{}

		environment_t &
		environment() const noexcept override
			{
				return m_owner->so_environment();
			}

		priority_t
		sink_priority() const noexcept override
			{
				return m_owner->so_priority();
			}

		void
		push_event(
			mbox_id_t mbox_id,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep,
			const message_limit::impl::action_msg_tracer_t * tracer )
			{
				if( m_control_block.m_limit < ++(m_control_block.m_count) )
					{
						--(m_control_block.m_count);

						m_control_block.m_action(
								so_5::message_limit::overlimit_context_t{
										mbox_id,
										*m_owner,
										m_control_block,
										overlimit_reaction_deep,
										msg_type,
										message,
										tracer
								} );
					}
				else
					{
						using namespace push_event_impl;

						decrement_on_exception_t exception_guard(
								std::addressof( m_control_block ) );

						agent_t::call_push_event(
								*m_owner,
								std::addressof( m_control_block ),
								mbox_id,
								msg_type,
								message );

						exception_guard.commit();
					}
			}
	};

//
// sinks_storage_t
//
/*!
 * \brief An interface for storage of message_sinks for one agent.
 *
 * \since v.5.8.0
 */
class sinks_storage_t
	{
		sinks_storage_t( const sinks_storage_t & ) = delete;
		sinks_storage_t & operator=( const sinks_storage_t & ) = delete;

	public :
		sinks_storage_t() = default;
		virtual ~sinks_storage_t() = default;

		//FIXME: is it really needed?
		[[nodiscard]]
		virtual message_sink_t *
		find( const std::type_index & msg_type ) = 0;

		[[nodiscard]]
		virtual message_sink_t *
		find_or_create( const std::type_index & msg_type ) = 0;
	};

//
// storage_without_limits_t
//
/*!
 * \brief Implementation of message sink storage for a case when
 * message limits are not used.
 *
 * It holds just one sink that will be used for all types of messages.
 *
 * \since v.5.8.0
 */
class storage_without_limits_t final
	: public sinks_storage_t
	{
		//! The single sinks that is needed if message limits are not used.
		message_sink_without_message_limit_t m_sink;

	public:
		storage_without_limits_t(
			partially_constructed_agent_ptr_t owner_ptr )
			:	m_sink{ owner_ptr }
			{}

		message_sink_t *
		find( const std::type_index & /*msg_type*/ ) override
			{
				return std::addressof( m_sink );
			}

		message_sink_t *
		find_or_create( const std::type_index & /*msg_type*/ ) override
			{
				return std::addressof( m_sink );
			}
	};

//
// message_sinks_with_limits_holder_t
//
//FIXME: document this!
class message_sinks_with_limits_holder_t
	{
		//! Type of storage for message_sinks.
		using storage_t = std::map<
				std::type_index,
				message_sink_with_message_limit_t >;

		//! All created message_sinks.
		storage_t m_sinks;

		//! Helper for build an initial storage.
		[[nodiscard]]
		static storage_t
		build_sinks(
			partially_constructed_agent_ptr_t owner_ptr,
			so_5::message_limit::description_container_t && descriptions )
			{
				storage_t result;

				for( auto & d : descriptions )
					result.emplace( d.m_msg_type,
							message_sink_with_message_limit_t{
									owner_ptr,
									d.m_limit,
									std::move(d.m_action)
							} );

				return result;
			}

	public:
		//! Initializing constructor.
		message_sinks_with_limits_holder_t(
				partially_constructed_agent_ptr_t owner_ptr,
				so_5::message_limit::description_container_t && descriptions )
				:	m_sinks{ build_sinks( owner_ptr, std::move(descriptions) ) }
			{}

		[[nodiscard]]
		message_sink_t *
		find( const std::type_index & msg_type )
			{
				auto it = m_sinks.find( msg_type );
				if( it == m_sinks.end() )
					return nullptr;

				return std::addressof( it->second );
			}

		[[nodiscard]]
		message_sink_t *
		find_or_create(
			const std::type_index & msg_type,
			partially_constructed_agent_ptr_t owner_ptr,
			const so_5::message_limit::description_t & limit_description )
			{
				auto it = m_sinks.find( msg_type );
				if( it == m_sinks.end() )
					{
						it = m_sinks.emplace(
								msg_type,
								message_sink_with_message_limit_t{
										owner_ptr,
										limit_description.m_limit,
										limit_description.m_action
								} )
								.first;
					}

				return std::addressof( it->second );
			}
	};

//
// fixed_sinks_storage_t
//
/*!
 * \brief A fixed-capacity storage for message sinks with
 * message limits for one agent.
 *
 * \since v.5.8.0
 */
class fixed_sinks_storage_t final : public sinks_storage_t
	{
	public :
		//! Initializing constructor.
		fixed_sinks_storage_t(
			//! Owner for all sinks to be constructed.
			partially_constructed_agent_ptr_t owner_ptr,
			//! Source description of limits.
			//! Since v.5.7.1 this container is expected to be
			//! sorted and checked for duplicates.
			so_5::message_limit::description_container_t && descriptions )
			:	m_holder{ owner_ptr, std::move(descriptions) }
			{}

		[[nodiscard]]
		message_sink_t *
		find( const std::type_index & msg_type ) override
			{
				return m_holder.find( msg_type );
			}

		[[nodiscard]]
		message_sink_t *
		find_or_create( const std::type_index & msg_type ) override
			{
				return m_holder.find( msg_type );
			}

	private :
		//! All sinks.
		message_sinks_with_limits_holder_t m_holder;
	};

//
// growable_sinks_storage_t
//
/*!
 * \brief A storage of growable capacity for message limits for one agent.
 *
 * \since v.5.8.0
 */
class growable_sinks_storage_t final : public sinks_storage_t
	{
	public :
		//! Initializing constructor.
		growable_sinks_storage_t(
			//! Owner for all sinks to be constructed.
			partially_constructed_agent_ptr_t owner_ptr,
			//! Description of the default limit.
			so_5::message_limit::description_t && default_limit_description,
			//! Source description of limits.
			so_5::message_limit::description_container_t && descriptions )
			:	m_owner_ptr{ owner_ptr }
			,	m_default_limit_description{ std::move(default_limit_description) }
			,	m_holder{ owner_ptr, std::move(descriptions) }
			{}

		[[nodiscard]]
		message_sink_t *
		find( const std::type_index & msg_type ) override
			{
				return m_holder.find( msg_type );
			}

		[[nodiscard]]
		message_sink_t *
		find_or_create( const std::type_index & msg_type ) override
			{
				return m_holder.find_or_create(
						msg_type,
						m_owner_ptr,
						m_default_limit_description );
			}

	private :
		//! Owner of sinks and limits.
		const partially_constructed_agent_ptr_t m_owner_ptr;

		//! Description of the default limit.
		const so_5::message_limit::description_t m_default_limit_description;

		//! All sinks and limits.
		message_sinks_with_limits_holder_t m_holder;
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
inline std::tuple<
		so_5::message_limit::description_container_t,
		std::optional< so_5::message_limit::description_t > >
prepare(
	so_5::message_limit::description_container_t original_descriptions )
{
	using namespace std;

	optional< so_5::message_limit::description_t > default_limit;

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
	const std::type_index mark_type{
			typeid(so_5::message_limit::any_unspecified_message)
		};
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
inline static std::unique_ptr< sinks_storage_t >
create_sinks_storage_if_necessary(
	partially_constructed_agent_ptr_t owner_ptr,
	so_5::message_limit::description_container_t && descriptions )
	{
		std::unique_ptr< sinks_storage_t > result;

		if( descriptions.empty() )
			{
				result = std::make_unique< storage_without_limits_t >(
						owner_ptr );
			}
		else
			{
				auto [sorted_descs, default_limit] =
						description_preparation_details::prepare(
								std::move(descriptions) );

				if( !default_limit )
					// There is no default limit, so fixed_sinks_storage_t
					// should be used.
					result = std::make_unique< fixed_sinks_storage_t >(
							owner_ptr,
							std::move(sorted_descs) );
				else
					result = std::make_unique< growable_sinks_storage_t >(
							owner_ptr,
							std::move(*default_limit),
							std::move(sorted_descs) );
			}

		return result;
	}

} /* namespace impl */

} /* namespace so_5 */

