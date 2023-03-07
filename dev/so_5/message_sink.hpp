/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A base class for message sinks.
	\since v.5.8.0
*/

#pragma once

#include <so_5/message.hpp>
#include <so_5/priority.hpp>

namespace so_5
{

//
// abstract_message_sink_t
//
//FIXME: document this!
class SO_5_TYPE abstract_message_sink_t
	{
	public:
		virtual ~abstract_message_sink_t() noexcept = default;

		[[nodiscard]]
		virtual environment_t &
		environment() const noexcept = 0;

		[[nodiscard]]
		virtual priority_t
		sink_priority() const noexcept = 0;

		virtual void
		push_event(
			mbox_id_t mbox_id,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep,
			//! Message delivery tracer to be used inside overlimit reaction.
			const message_limit::impl::action_msg_tracer_t * tracer ) = 0;
	};

//
// abstract_sink_owner_t
//
//FIXME: document this!
class SO_5_TYPE abstract_sink_owner_t : protected atomic_refcounted_t
	{
		friend class intrusive_ptr_t< abstract_sink_owner_t >;

	public:
		abstract_sink_owner_t() = default;
		virtual ~abstract_sink_owner_t() noexcept = default;

		[[nodiscard]]
		virtual abstract_message_sink_t &
		sink() noexcept = 0;

		[[nodiscard]]
		virtual const abstract_message_sink_t &
		sink() const noexcept = 0;
	};

//
// msink_t
//
//FIME: document this!
using msink_t = intrusive_ptr_t< abstract_sink_owner_t >;

//
// simple_sink_owner_t
//
//FIXME: document this!
template< typename Sink_Type >
class simple_sink_owner_t final : public abstract_sink_owner_t
	{
		Sink_Type m_sink;

	public:
		/*!
		 * \brief Initializing constructor.
		 *
		 * Forwards all arguments to the m_sink's constructor.
		 */
		template< typename... Args >
		simple_sink_owner_t( Args && ...args )
			:	m_sink{ std::forward<Args>(args)... }
			{}

		[[nodiscard]]
		abstract_message_sink_t &
		sink() noexcept override
			{
				return m_sink;
			}

		[[nodiscard]]
		const abstract_message_sink_t &
		sink() const noexcept override
			{
				return m_sink;
			}
	};

} /* namespace so_5 */

