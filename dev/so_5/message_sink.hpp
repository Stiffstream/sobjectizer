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

#include <functional>

namespace so_5
{

//
// max_redirection_deep
//
/*!
 * \brief Maximum deep of message redirections.
 *
 * \since v.5.8.0
 */
constexpr unsigned int max_redirection_deep = 32;

//
// abstract_message_sink_t
//
//FIXME: document this!
class SO_5_TYPE abstract_message_sink_t
	{
	public:
		abstract_message_sink_t() = default;
		virtual ~abstract_message_sink_t() noexcept = default;

		abstract_message_sink_t(
				const abstract_message_sink_t & ) = default;
		abstract_message_sink_t &
		operator=(
				const abstract_message_sink_t & ) = default;

		abstract_message_sink_t(
				abstract_message_sink_t && ) = default;
		abstract_message_sink_t &
		operator=(
				abstract_message_sink_t && ) = default;

		[[nodiscard]]
		virtual environment_t &
		environment() const noexcept = 0;

		[[nodiscard]]
		virtual priority_t
		sink_priority() const noexcept = 0;

		virtual void
		push_event(
			mbox_id_t mbox_id,
			message_delivery_mode_t delivery_mode,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int redirection_deep,
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

namespace impl
{

//
// msink_less_comparator_t
//
//FIXME: document this!
struct msink_less_comparator_t
	{
		[[nodiscard]]
		static std::pair< const abstract_sink_owner_t *, so_5::priority_t >
		safe_get_pair( const msink_t & from ) noexcept
			{
				if( from )
					return { from.get(), from->sink().sink_priority() };
				else
					return { nullptr, so_5::prio::p0 };
			}

		[[nodiscard]]
		bool
		operator()( const msink_t & a, const msink_t & b ) const noexcept
			{
				//FIXME: document this logic!
				const std::less< const abstract_sink_owner_t * > ptr_less;
				const std::less< so_5::priority_t > prio_less;

				const auto [a_ptr, a_prio] = safe_get_pair( a );
				const auto [b_ptr, b_prio] = safe_get_pair( b );

				if( ptr_less( a_ptr, b_ptr ) )
					return true;
				else if( ptr_less( b_ptr, a_ptr ) )
					return false;
				else return prio_less( a_prio, b_prio );
			}
	};

//
// msink_const_ref_for_comparison_t
//
struct msink_const_ref_for_comparison_t
	{
		const msink_t & m_ref;
	};

//FIXME: document this!
[[nodiscard]]
inline bool
operator<(
	const msink_const_ref_for_comparison_t & a,
	const msink_const_ref_for_comparison_t & b ) noexcept
	{
		return impl::msink_less_comparator_t{}( a.m_ref, b.m_ref );
	}

} /* namespace impl */

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

