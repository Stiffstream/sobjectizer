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
 * Examples for message redirections:
 *
 * - message limits are used and so_5::agent_t::limit_then_redirect is
 *   used for pushing a message to another mbox;
 * - a custom message sink is used for transfering messages from
 *   one mbox to another.
 *
 * Such redirections could lead to infinite loops.
 * SObjectizer cannot detect such loops so it uses a primitive protection
 * method: the limit for a number of redirections. If message delivery
 * attempt exceeds this limit then the delivery must be cancelled
 * (with or without an exception).
 *
 * \since v.5.8.0
 */
constexpr unsigned int max_redirection_deep = 32;

//
// abstract_message_sink_t
//
/*!
 * \brief Interface for message sink.
 *
 * This class is the base for all message sinks.
 *
 * The purpose of message sink is to be a subscriber for an mbox. An mbox holds
 * a list of subscribers and delivers a message to appropriate subscribers
 * (message sinks). When an mbox receives a message it calls push_event()
 * method for all message sinks that are subscribed to this message
 * (and delivery filters permit the delivery to those sinks).
 *
 * \note
 * A message sink in SO-5.8 plays the same role as agent_t in previous
 * versions, in the sense that message sinks act as a receiver of messages sent
 * to message boxes. Since an agent_t has priority and this priority is taken
 * into account during subscriptions and message delivery, the message sink
 * should also have priority. Because of that there is sink_priority() method.
 * For cases where a message sink is created for an agent, the sink_priority()
 * should return the agent's priority.
 *
 * \since v.5.8.0
 */
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
				abstract_message_sink_t && ) noexcept = default;
		abstract_message_sink_t &
		operator=(
				abstract_message_sink_t && ) noexcept = default;

		//! Get a reference for SObjectizer Environment for that
		//! the message sink is created.
		/*!
		 * This method plays the same role as abstract_message_box_t::environment()
		 * or coop_t::environment().
		 */
		[[nodiscard]]
		virtual environment_t &
		environment() const noexcept = 0;

		//! Get the priority for the message sink.
		[[nodiscard]]
		virtual priority_t
		sink_priority() const noexcept = 0;

		//! Get a message and push it to the appropriate destination.
		/*!
		 * This is key method for message sink. Its logic depends of message sink
		 * type. For example, ordinary message sink for an agent just pushes the
		 * message to the agent's event_queue.
		 *
		 * \attention
		 * Implementations of message sinks should control the value of
		 * \a redirection_deep. If that value exceeds \a so_5::max_redirection_deep
		 * then the delivery procedure has to be cancelled and the message
		 * (signal) should be dropped (ignored). If an implementation redirects
		 * message to another mbox/msink then the value of \a redirection_deep
		 * has to be incremented.
		 *
		 * \attention
		 * The \a tracer can be nullptr. Moreover, it will be nullptr in most of
		 * the cases when message delivery tracing is off.
		 */
		virtual void
		push_event(
			//! ID of mbox from that the message is received.
			mbox_id_t mbox_id,
			//! Delivery mode for this delivery attempt.
			message_delivery_mode_t delivery_mode,
			//! Type of message to be delivered.
			const std::type_index & msg_type,
			//! Reference to the message to be delivered.
			//! NOTE: it can be empty (nullptr) reference in the case of a signal.
			const message_ref_t & message,
			//! The current deep of message redirection between mboxes and msinks.
			unsigned int redirection_deep,
			//! Message delivery tracer to be used inside overlimit reaction.
			//! NOTE: it will be nullptr when message delivery tracing if off.
			const message_limit::impl::action_msg_tracer_t * tracer ) = 0;

		[[nodiscard]]
		static bool
		special_sink_ptr_compare(
			const abstract_message_sink_t * a,
			const abstract_message_sink_t * b ) noexcept
			{
				// Use std::less for compare pointers.
				// NOTE: it's UB to compare pointers that do not belong to the same array.
				using ptr_comparator_t = std::less< const abstract_message_sink_t * >;

				const auto priority_getter = [](auto * sink) noexcept {
						return sink == nullptr ? so_5::priority_t::p_min : sink->sink_priority();
					};
				const auto p1 = priority_getter( a );
				const auto p2 = priority_getter( b );

				// NOTE: there should be inversion -- sink with higher
				// priority must be first.
				return ( p1 > p2 || (p1 == p2 && ptr_comparator_t{}( a, b ) ) );
			}
	};

//
// abstract_sink_owner_t
//
/*!
 * \brief Interface for holders of message_sink instances.
 *
 * There is mbox_t, which is a kind of smart pointer to abstract_message_box_t.
 * It's a very useful type that allows to hold references to message boxes
 * safely. But mbox_t exists because all message boxes should be dynamically
 * allocated objects. The situation with message sinks is a bit more
 * complicated. Not all message sinks are dynamically allocated, some may be
 * parts of other objects. But if a message sink is dynamically allocated then
 * it's good to have something like mbox_t, but for message sinks.
 *
 * The type abstract_sink_owner_t is a proxy for a real message sink object.
 * Instances of abstract_sink_owner_t are always allocated dynamically. They
 * can hold real message sinks as members (in this case abstract_sink_owner
 * lives on the heap and the message sink lives inside it), or they can hold
 * references to message sinks that live elsewhere.
 *
 * The presence of abstract_sink_owner_t allows to have msink_t type, which is
 * very similar to mbox_t.
 *
 * Note that the abstract_sink_owner_t is an abstract class. The real
 * implementations of sink owners must be derived from this class.
 *
 * \note
 * It's expected that one abstract_sink_owner_t holds a reference to
 * just one message sink.
 *
 * \since v.5.8.0
 */
class SO_5_TYPE abstract_sink_owner_t : protected atomic_refcounted_t
	{
		friend class intrusive_ptr_t< abstract_sink_owner_t >;

	public:
		abstract_sink_owner_t() = default;
		virtual ~abstract_sink_owner_t() noexcept = default;

		//! Get a reference to the underlying message sink.
		[[nodiscard]]
		virtual abstract_message_sink_t &
		sink() noexcept = 0;

		//! Get a const reference to the underlying message sink.
		[[nodiscard]]
		virtual const abstract_message_sink_t &
		sink() const noexcept = 0;
	};

//
// msink_t
//
/*!
 * \brief Smart reference for abstract_sink_owner.
 *
 * \since v.5.8.0
 */
using msink_t = intrusive_ptr_t< abstract_sink_owner_t >;

namespace impl
{

//
// msink_less_comparator_t
//
/*!
 * \brief Helper class to be used as a comparator for msinks.
 *
 * Two instances of msink_t can't be compared in a trivial way: they can
 * be nullptr, if both are not nullptr then their priorities have to be
 * compared too.
 *
 * This class implements the logic of msink_t comparison and can be used
 * as Comparator template parameter for std::set/map classes. For example:
 * \code
 * std::set<so_5::msink_t, so_5::impl::msink_less_comparator_t> known_msinks;
 * \endcode
 *
 * \attention
 * This class is part of the SObjectizer implementation and is subject to
 * change without notice.
 *
 * \since v.5.8.0
 */
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
				const std::less< const abstract_sink_owner_t * > ptr_less;
				const std::less< so_5::priority_t > prio_less;

				const auto [a_ptr, a_prio] = safe_get_pair( a );
				const auto [b_ptr, b_prio] = safe_get_pair( b );

				if( ptr_less( a_ptr, b_ptr ) )
					return true;
				else if( ptr_less( b_ptr, a_ptr ) )
					return false;
				// Assume that a_ptr == b_ptr.
				else return prio_less( a_prio, b_prio );
			}
	};

} /* namespace impl */

//
// simple_sink_owner_t
//
/*!
 * \brief Implementation of abstract_sink_owner that owns an instance
 * of message sink.
 *
 * \tparam Sink_Type Type of the message sink to be created as a part of owner object.
 *
 * \since v.5.8.0
 */
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

