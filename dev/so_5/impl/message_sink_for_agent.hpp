/*
 * SObjectizer-5
 */

/*!
 * \since v.5.8.0
 *
 * \brief A base class for message sinks to be used by agents.
 */

#pragma once

#include <so_5/message_limit.hpp>

namespace so_5
{

namespace impl
{

//
// message_sink_for_agent_t
//
/*!
 * \brief A base class for message sinks to be used by agents.
 *
 * \note
 * It's abstract class, it doesn't implement push() method.
 * But it introduces owner() method.
 *
 * \since v.5.8.0
 */
class message_sink_for_agent_t
	: public abstract_message_sink_t
	{
		//! Owner of the sink.
		agent_t & m_owner;

	public:
		//! Constructor for a case when agent is being created.
		message_sink_for_agent_t(
			partially_constructed_agent_ptr_t owner_ptr )
			:	m_owner( *(owner_ptr.ptr()) )
			{}

		//! Constructor for a case when agent is already created.
		message_sink_for_agent_t(
			outliving_reference_t< agent_t > owner )
			:	m_owner( owner.get() )
			{}

		environment_t &
		environment() const noexcept override
			{
				return m_owner.so_environment();
			}

		priority_t
		sink_priority() const noexcept override
			{
				return m_owner.so_priority();
			}

		[[nodiscard]]
		agent_t &
		owner_reference() const noexcept
			{
				return m_owner;
			}

		[[nodiscard]]
		agent_t *
		owner_pointer() const noexcept
			{
				return std::addressof(m_owner);
			}
	};

} /* namespace impl */

} /* namespace so_5 */


