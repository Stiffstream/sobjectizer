/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A base class for the agent messages definition.
*/

#pragma once

#include <so_5/declspec.hpp>
#include <so_5/exception.hpp>
#include <so_5/atomic_refcounted.hpp>
#include <so_5/types.hpp>

#include <so_5/agent_ref_fwd.hpp>

#include <type_traits>
#include <typeindex>
#include <functional>
#include <atomic>
#include <optional>

namespace so_5
{

// This forward declaration is needed for message_upcaster_t.
class message_t;

//
// message_ref_t
//
//! A smart reference to the message.
/*!
 * \note Defined as typedef since v.5.2.0
 */
using message_ref_t = intrusive_ptr_t< message_t >;

//
// message_upcaster_t
//
//FIXME: document this!
class message_upcaster_t
{
public:
	using base_type_getter_t =
			std::type_index (*)() noexcept;
	using actual_type_getter_t =
			std::type_index (*)() noexcept;
	using next_upcaster_getter_t =
			std::optional<message_upcaster_t> (*)(const message_ref_t &) noexcept;

	message_upcaster_t(
		base_type_getter_t base_type_getter,
		actual_type_getter_t actual_type_getter,
		next_upcaster_getter_t next_upcaster_getter )
		:	m_base_type_getter{ base_type_getter }
		,	m_actual_type_getter{ actual_type_getter }
		,	m_next_upcaster_getter{ next_upcaster_getter }
	{}

	[[nodiscard]]
	std::type_index
	base_type() const noexcept
	{
		return (*m_base_type_getter)();
	}

	[[nodiscard]]
	std::type_index
	actual_type() const noexcept
	{
		return (*m_actual_type_getter)();
	}

	[[nodiscard]]
	std::optional<message_upcaster_t>
	next_upcaster( const message_ref_t & msg ) const noexcept
	{
		return (*m_next_upcaster_getter)( msg );
	}

private:
	base_type_getter_t m_base_type_getter;
	actual_type_getter_t m_actual_type_getter;
	next_upcaster_getter_t m_next_upcaster_getter;
};

//
// message_t
//

//! A base class for agent messages.
/*!
 * All messages for agents must be derived from this class.
 *
 * \attention This class should be used for all messages which
 * have an actual message data. For signals (messages without any data)
 * a signal_t class should be used as a base class.
 *
 * \note
 * Class message_t is derived from atomic_refcounted_t.
 * But atomic_refcounted_t is not Copyable or Movable class.
 * It means that copy/move constructors and operators for message_t
 * have no influence on the reference counter inside message_t.
 */
class SO_5_TYPE message_t : public atomic_refcounted_t
	{
		friend class impl::internal_message_iface_t;
	public:
		//! A short typedef for kind of message.
		/*!
		 * \since
		 * v.5.5.23
		 */
		using kind_t = ::so_5::message_kind_t;

		message_t();
		message_t( const message_t & other );
		message_t( message_t && other );

		message_t &
		operator=( const message_t & other );
		message_t &
		operator=( message_t && other );

		virtual ~message_t() noexcept = default;

		/*!
		 * \brief Helper method for safe get of message mutability flag.
		 *
		 * Use this method instead of direct call to so5_message_mutability()
		 * because \a what will be nullptr for signals.
		 */
		friend message_mutability_t
		message_mutability( const intrusive_ptr_t<message_t> & what ) noexcept
			{
				message_mutability_t r = message_mutability_t::immutable_message;
				if( what )
					r = what->so5_message_mutability();
				return r;
			}

		/*!
		 * \brief Helper method for get message mutability flag.
		 */
		friend message_mutability_t
		message_mutability( const message_t & what ) noexcept
			{
				return what.so5_message_mutability();
			}

		/*!
		 * \brief Helper method for safe change message mutability flag.
		 *
		 * Use this method instead of direct call to so5_change_mutability()
		 * because \a what will be nullptr for signals.
		 *
		 * \note
		 * This is a very dangerous operation. Don't do it by yourselt.
		 * See message_t::so5_change_mutability() for more details.
		 *
		 * \attention
		 * This function can throw.
		 * For example underlying message object can prohibit changing
		 * of message mutability.
		 */
		friend void
		change_message_mutability(
			//! Message instance to be modified if it is not a signal.
			intrusive_ptr_t< message_t> & what,
			//! New mutability flag for the message.
			message_mutability_t mutability )
			{
				if( what )
					what->so5_change_mutability( mutability );
			}

		/*!
		 * \brief Helper method for change message mutability flag.
		 *
		 * \note
		 * This is a very dangerous operation. Don't do it by yourselt.
		 * See message_t::so5_change_mutability() for more details.
		 *
		 * \attention
		 * This function can throw.
		 * For example underlying message object can prohibit changing
		 * of message mutability.
		 */
		friend void
		change_message_mutability(
			//! Message instance to be modified.
			message_t & what,
			//! New mutability flag for the message.
			message_mutability_t mutability )
			{
				what.so5_change_mutability( mutability );
			}

		/*!
		 * \brief Helper method for quering kind of the message.
		 *
		 * This helper function is necessary because it correctly
		 * handles case when message is a signal. In that case pointer
		 * to message instance will be null.
		 *
		 * \note
		 * This function is part of internal implementation of SObjectizer.
		 * Don't use it directly. It can be a subject of changes in some
		 * future versions.
		 *
		 * \since
		 * v.5.5.23
		 */
		friend message_kind_t
		message_kind(
			const so_5::intrusive_ptr_t< message_t > & what )
			{
				if( what )
					return what->so5_message_kind();
				else
					return message_kind_t::signal;
			}

		/*!
		 * \brief Helper method for quering kind of the message.
		 *
		 * \note
		 * This function is part of internal implementation of SObjectizer.
		 * Don't use it directly. It can be a subject of changes in some
		 * future versions.
		 *
		 * \since
		 * v.5.5.23
		 */
		friend message_kind_t
		message_kind(
			const message_t & what )
			{
				return what.so5_message_kind();
			}

		//FIXME: document this!
		[[nodiscard]]
		virtual std::optional<message_upcaster_t>
		so_message_upcaster() const noexcept;

	private :
		/*!
		 * \brief Is message mutable or immutable?
		 *
		 * By default the message is immutable.
		 * \since
		 * v.5.5.19
		 */
		message_mutability_t m_mutability;

		/*!
		 * \brief Get message mutability flag.
		 *
		 * \note
		 * This method is intended to be used by SObjectizer and
		 * low-level SObjectizer extension. Because of that it is not
		 * guaranteed that this method is part of stable SObjectizer API.
		 * It can be changed or even removed in any future versions
		 * of SObjectizer.
		 *
		 * \note
		 * This is a virual method because its behavious must be changed in
		 * msg_service_request_t.
		 *
		 * \since
		 * v.5.5.19
		 */
		virtual message_mutability_t
		so5_message_mutability() const noexcept { return m_mutability; }

		/*!
		 * \brief Change message mutabilty flag.
		 *
		 * \attention
		 * Changing mutability from message_mutability_t::immutable_message
		 * to message_mutability_t::mutable_message is a very bad idea.
		 * Please don't do this until you are know what you are doing.
		 *
		 * \note
		 * This method is intended to be used by SObjectizer and
		 * low-level SObjectizer extension. Because of that it is not
		 * guaranteed that this method is part of stable SObjectizer API.
		 * It can be changed or even removed in any future versions
		 * of SObjectizer.
		 *
		 * \note
		 * This is a virual method because its behavious must be changed in
		 * msg_service_request_t.
		 *
		 * \attention
		 * This function can throw.
		 * For example a derived class can prohibit changing
		 * of message mutability.
		 *
		 * \since
		 * v.5.5.19
		 */
		virtual void
		so5_change_mutability(
			//! New mutability flag for
			message_mutability_t mutability )
			{
				m_mutability = mutability;
			}

		/*!
		 * \brief Detect the kind of the message.
		 *
		 * \note
		 * This method is intended to be used by SObjectizer and
		 * low-level SObjectizer extension. Because of that it is not
		 * guaranteed that this method is part of stable SObjectizer API.
		 * It can be changed or even removed in any future versions
		 * of SObjectizer.
		 *
		 * \since
		 * v.5.5.23
		 */
		virtual kind_t
		so5_message_kind() const noexcept
			{
				return kind_t::classical_message;
			}
	};

//
// signal_t
//
//! A base class for agent signals.
/*!
 * \since
 * v.5.2.0
 *
 * All signals (messages without any data) for agents should be
 * derived from this class.
 */
class SO_5_TYPE signal_t
	:	public message_t
{
	private :
		// Note: clang-3.9 requires this on Windows platform.
		signal_t( const signal_t & ) = delete;
		signal_t( signal_t && ) = delete;
		signal_t & operator=( const signal_t & ) = delete;
		signal_t & operator=( signal_t && ) = delete;

		/*!
		 * Private constructor to disable creation of instances of
		 * derived classes.
		 */
		signal_t() = default;

		// Note: this method has no sence in v.5.5, becasue instances
		// of that class are not created at all.
		// But that can be changed in v.5.6.0.
		kind_t
		so5_message_kind() const noexcept override
			{
				return kind_t::classical_message;
			}

	public :
		~signal_t() noexcept override = default;
};

//
// user_type_message_t
//
/*!
 * \brief Template class for representing object of user type as a message.
 *
 * \tparam T type of actual message. This type must have move- or copy
 * constructor.
 *
 * \since
 * v.5.5.9
 */
template< typename T >
struct user_type_message_t : public message_t
{
	//! Instance of user message.
	/*!
	 * \note
	 * Since v.5.5.19 it is not a const object anymore.
	 */
	T m_payload;

	//! Initializing constructor.
	template< typename... Args >
	user_type_message_t( Args &&... args )
		:	m_payload{ std::forward< Args >( args )... }
		{}

	//! Initialization from const T object.
	user_type_message_t( const T & o )
		:	m_payload{ o }
		{}

	//! Initialization from non-const T object.
	user_type_message_t( T & o )
		:	m_payload{ o }
		{}

	//! Initialization from temporary T object.
	user_type_message_t( T && o )
		:	m_payload{ std::move(o) }
		{}

private :
	kind_t
	so5_message_kind() const noexcept override
		{
			return kind_t::user_type_message;
		}
};

//
// immutable_msg
//
/*!
 * \brief A special marker for immutable message.
 *
 * This marker tells that message can be sent to several receivers and
 * nobody can change the message content.
 *
 * \tparam M type of the message.
 *
 * \since
 * v.5.5.19
 */
template< typename M >
struct immutable_msg final {};

//
// mutable_msg
//
/*!
 * \brief A special marker for mutable message.
 *
 * This marker tells that message can be sent only for one receiver.
 * And that receiver will get an exclusive access to the message content.
 * It means that receiver can change message's content. 
 *
 * \tparam M type of the message.
 *
 * \since
 * v.5.5.19
 */
template< typename M >
struct mutable_msg final {};

namespace details {

//
// message_mutability_traits
//
/*!
 * \brief Detector of message type traits in dependency of message immutability
 * or mutability.
 *
 * \since
 * v.5.5.19
 */
template< typename T >
struct message_mutability_traits
	{
		using payload_type = typename std::decay<T>::type;
		using subscription_type = payload_type;
		using mhood_param_type = payload_type;

		static const constexpr message_mutability_t mutability =
				message_mutability_t::immutable_message;
	};

template< typename T >
struct message_mutability_traits< immutable_msg<T> >
	{
		using payload_type = T;
		using subscription_type = T;
		using mhood_param_type = T;

		static const constexpr message_mutability_t mutability =
				message_mutability_t::immutable_message;
	};

template< typename T >
struct message_mutability_traits< mutable_msg<T> >
	{
		using payload_type = T;
		using subscription_type = mutable_msg<T>;
		using mhood_param_type = mutable_msg<T>;

		static const constexpr message_mutability_t mutability =
				message_mutability_t::mutable_message;
	};

} /* namespace details */

//
// is_user_type_message
//
/*!
 * \since
 * v.5.5.9
 *
 * \brief A helper for detection presence of message of user type.
 *
 * \tparam M type to test.
 */
template< typename M >
struct is_user_type_message
	{
		enum { value = false };
	};

template< typename M >
struct is_user_type_message< user_type_message_t< M > >
	{
		enum { value = true };
	};

//
// is_signal
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief A helper class for checking that message is a signal.
 */
template< class T >
struct is_signal
	{
		enum { value = std::is_base_of<
				signal_t,
				typename details::message_mutability_traits<T>::payload_type >::value };
	};

//
// is_classical_message
//
/*!
 * \since
 * v.5.5.9
 *
 * \brief A helper class for checking that message is a classical message
 * derived from %message_t class.
 */
template< class T >
struct is_classical_message
	{
		enum { value = std::is_base_of<
				message_t,
				typename details::message_mutability_traits<T>::payload_type >::value };
	};

//
// is_mutable_message
//
/*!
 * \brief A helper class for checking that message is a mutable message.
 *
 * \since
 * v.5.5.19
 */
template< typename T >
struct is_mutable_message
	{
		enum { value = false };
	};

template< typename T >
struct is_mutable_message< mutable_msg<T> >
	{
		enum { value = true };
	};

//
// ensure_not_signal
//
/*!
 * \since
 * v.5.2.0
 *
 * \brief A special compile-time checker to guarantee that the message
 * class is not a signal class.
 */
template< class Msg >
void
ensure_not_signal()
{
	static_assert( !is_signal< Msg >::value,
			"message class must be derived from the message_t" );
}

//
// ensure_message_with_actual_data
//
/*!
 * \since
 * v.5.2.0
 *
 * \brief A special checker to guarantee that the message is an instance
 * of the message_t (not signal_t) and has a not-null pointer 
 * to the message data.
 *
 * \note A check for the inheritance from the message_t is done at compile-time.
 *
 * \tparam Msg message type to be checked.
 */
template< class Msg >
void
ensure_message_with_actual_data( const Msg * m )
{
	ensure_not_signal< Msg >();

	if( !m )
		throw so_5::exception_t(
				"an attempt to send a message via nullptr",
				so_5::rc_null_message_data );
}

//
// ensure_not_mutable_signal
//
/*!
 * \brief A special compile-time checker to guarantee that S is not a
 * mutable signal.
 *
 * This check is intended to prevent usage of mutable_msg<S> where S is 
 * a signal type.
 *
 * \since
 * v.5.5.19
 */
template< class S >
void
ensure_not_mutable_signal()
{
	static_assert( !is_signal<S>::value ||
			message_mutability_t::immutable_message ==
					details::message_mutability_traits<S>::mutability,
			"usage of mutable_msg<S> where S is a signal is prohibited" );
}

//
// ensure_signal
//
/*!
 * \since
 * v.5.2.0
 *
 * \brief A special compile-time checker to guarantee that the Msg is derived
 * from the signal_t.
 *
 * \tparam Msg signal type to be checked.
 */
template< class Msg >
void
ensure_signal()
{
	static_assert( is_signal< Msg >::value,
			"expected a type derived from the signal_t" );

	// Added in v.5.5.19.
	// Msg must not be a mutable_msg<S>.
	ensure_not_mutable_signal<Msg>();
}

//
// ensure_classical_message
//
/*!
 * \since
 * v.5.5.9
 *
 * \brief A special compile-time checker to guarantee that Msg is derived from
 * %message_t.
 *
 * \tparam Msg type to be checked.
 */
template< typename Msg >
void
ensure_classical_message()
	{
		static_assert( is_classical_message< Msg >::value,
				"expected a type derived from the message_t" );
	}

//
// message_payload_type_impl
//
/*!
 * \since
 * v.5.5.9
 *
 * \brief Implementation details for %message_payload_type.
 *
 * \note This specialization is for cases where T is derived from message_t.
 * In that case payload_type is the same as envelope_type.
 */
template< typename T, bool is_classical_message >
struct message_payload_type_impl
	{
		//! Mutability traits for this message type.
		using mutability_traits = details::message_mutability_traits<T>;

		//! Type visible to user.
		using payload_type = typename mutability_traits::payload_type;
		//! Type for message delivery.
		using envelope_type = payload_type;
		//! Type to which subscription must be done.
		using subscription_type = typename mutability_traits::subscription_type;

		//! Is it a signal type or message type.
		static constexpr const bool is_signal = ::so_5::is_signal<T>::value;

		//! Type ID for subscription.
		inline static std::type_index subscription_type_index()
			{
				// T must not be a mutable_msg<T>.
				ensure_not_mutable_signal<T>();

				return typeid(subscription_type);
			}

		//! Helper for extraction of pointer to payload part.
		/*!
		 * \note This method return non-const pointer because it is
		 * necessary for so_5::mhood_t.
		 */
		inline static payload_type *
		extract_payload_ptr( message_ref_t & msg )
			{
				return dynamic_cast< payload_type * >( msg.get() );
			}

		//! Helper for extraction of pointer to envelope part.
		/*!
		 * The same implementation as for extract_envelope_ptr().
		 */
		inline static envelope_type *
		extract_envelope_ptr( message_ref_t & msg )
			{
				return dynamic_cast< envelope_type * >( msg.get() );
			}

		//! Helper for getting a const reference to payload part.
		inline static payload_type &
		payload_reference( message_t & msg )
			{
				return dynamic_cast< payload_type & >( msg );
			}

		//! Helper for getting message mutability flag.
		inline static message_mutability_t
		mutability()
			{
				return mutability_traits::mutability;
			}
	};

/*!
 * \since
 * v.5.5.9
 *
 * \brief Implementation details for %message_payload_type.
 *
 * \note This specialization is for cases where T is not derived from message_t.
 * In that case payload_type is T, but envelope_type is user_type_message_t<T>.
 */
template< typename T >
struct message_payload_type_impl< T, false >
	{
		//! Mutability traits for this message type.
		using mutability_traits = details::message_mutability_traits<T>;

		//! Type visible to user.
		using payload_type = typename mutability_traits::payload_type;
		//! Type for message delivery.
		using envelope_type = user_type_message_t< payload_type >;
		//! Type to which subscription must be done.
		using subscription_type = typename mutability_traits::subscription_type;

		//! Is it a signal type or message type.
		/*!
		 * \note
		 * User-defined type can't be a signal.
		 */
		static constexpr const bool is_signal = false;

		//! Type ID for subscription.
		inline static std::type_index subscription_type_index()
			{
				return typeid(subscription_type);
			}

		//! Helper for extraction of pointer to payload part.
		/*!
		 * \note This method return const pointer because payload is
		 * a const object inside user_type_message_t<T> instance.
		 */
		inline static payload_type *
		extract_payload_ptr( message_ref_t & msg )
			{
				auto envelope = dynamic_cast< envelope_type * >( msg.get() );
				if( !envelope )
					SO_5_THROW_EXCEPTION( so_5::rc_unexpected_error,
							"nullptr for user_type_message_t<T> instance" );

				return &(envelope->m_payload);
			}

		//! Helper for extraction of pointer to envelope part.
		inline static envelope_type *
		extract_envelope_ptr( message_ref_t & msg )
			{
				return dynamic_cast< envelope_type * >( msg.get() );
			}

		//! Helper for getting a const reference to payload part.
		inline static payload_type &
		payload_reference( message_t & msg )
			{
				auto & envelope = dynamic_cast< envelope_type & >( msg );
				return envelope.m_payload;
			}

		//! Helper for getting message mutability flag.
		inline static message_mutability_t
		mutability()
			{
				return mutability_traits::mutability;
			}
	};

//
// message_payload_type
//
/*!
 * \since
 * v.5.5.9
 *
 * \brief A helper class for detection of payload type of message.
 *
 * \tparam T type to test.
 */
template< typename T >
struct message_payload_type
	:	public message_payload_type_impl< T,
			is_classical_message<
					typename ::so_5::details::message_mutability_traits<T>::payload_type >::value >
	{
	};

template< typename T >
struct message_payload_type< user_type_message_t< T > >
	:	public message_payload_type_impl< T, false >
	{
	};

namespace details
{

template< typename Msg >
typename std::enable_if<
	message_mutability_t::mutable_message ==
			::so_5::details::message_mutability_traits<Msg>::mutability >::type
mark_as_mutable_if_necessary( message_t & msg )
	{
		change_message_mutability( msg, message_mutability_t::mutable_message );
	}

/*!
 * \since
 * v.5.6.0
 */
template< typename Msg >
typename std::enable_if<
	message_mutability_t::mutable_message ==
			::so_5::details::message_mutability_traits<Msg>::mutability >::type
mark_as_mutable_if_necessary( message_ref_t & msg )
	{
		change_message_mutability( *msg, message_mutability_t::mutable_message );
	}

template< typename Msg >
typename std::enable_if<
	message_mutability_t::mutable_message !=
			::so_5::details::message_mutability_traits<Msg>::mutability >::type
mark_as_mutable_if_necessary( message_t & /*msg*/ )
	{
		// Nothing to do.
	}

/*!
 * \since
 * v.5.6.0
 */
template< typename Msg >
typename std::enable_if<
	message_mutability_t::mutable_message !=
			::so_5::details::message_mutability_traits<Msg>::mutability >::type
mark_as_mutable_if_necessary( message_ref_t & /*msg*/ )
	{
		// Nothing to do.
	}

template< bool is_signal, typename Msg >
struct make_message_instance_impl
	{
		using E = typename message_payload_type< Msg >::envelope_type;

		template< typename... Args >
		static std::unique_ptr< E >
		make( Args &&... args )
			{
				ensure_not_signal< Msg >();

				return std::unique_ptr< E >( new E( std::forward< Args >(args)... ) );
			}
	};

template< typename Msg >
struct make_message_instance_impl< true, Msg >
	{
		static std::unique_ptr< Msg >
		make()
			{
				ensure_signal< Msg >();

				return std::unique_ptr< Msg >();
			}
	};

/*!
 * \since
 * v.5.5.4
 *
 * \brief A helper for allocate instance of a message.
 */
template< typename Msg, typename... Args >
auto
make_message_instance( Args &&... args )
	-> std::unique_ptr< typename message_payload_type< Msg >::envelope_type >
	{
		return make_message_instance_impl<
						is_signal< Msg >::value, Msg
				>::make( std::forward< Args >( args )... );
	}

} /* namespace details */

namespace message_limit
{

struct control_block_t;

//
// any_unspecified_message
//

/*!
 * \brief A special mark to be used for default limits.
 *
 * \since
 * v.5.7.1
 */
class SO_5_TYPE any_unspecified_message final
	{
		// Instances of that type can't be created.
		~any_unspecified_message();
		any_unspecified_message();
	};

namespace impl
{

class action_msg_tracer_t;

} /* namespace impl */

/*!
 * \since
 * v.5.5.4
 *
 * \brief Description of context for overlimit action.
 */
struct overlimit_context_t
	{
		//! ID of mbox which is used for message delivery.
		/*!
		 * Added in v.5.5.23 because it is necessary for
		 * so_5::enveloped_msg::handling_context_t.
		 */
		const mbox_id_t m_mbox_id;

		//! Receiver of the message or service request.
		const agent_t & m_receiver;

		//! Control block for message limit.
		const control_block_t & m_limit;

		//! The current deep of overlimit reaction recursion.
		const unsigned int m_reaction_deep;

		//! Type of message to be delivered.
		const std::type_index & m_msg_type;

		//! A message or service request to be delivered.
		const message_ref_t & m_message;

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief An optional pointer to tracer object for
		 * message delivery tracing.
		 *
		 * \note Value nullptr means that message delivery tracing
		 * is not used.
		 */
		const impl::action_msg_tracer_t * m_msg_tracer;

		//! Initializing constructor.
		inline
		overlimit_context_t(
			mbox_id_t mbox_id,
			const agent_t & receiver,
			const control_block_t & limit,
			unsigned int reaction_deep,
			const std::type_index & msg_type,
			const message_ref_t & message,
			const impl::action_msg_tracer_t * msg_tracer )
			:	m_mbox_id( mbox_id )
			,	m_receiver( receiver )
			,	m_limit( limit )
			,	m_reaction_deep( reaction_deep )
			,	m_msg_type( msg_type )
			,	m_message( message )
			,	m_msg_tracer{ msg_tracer }
			{}
	};

//
// action_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief A type for reaction of message overlimit.
 */
using action_t = std::function< void(const overlimit_context_t&) >;

//
// control_block_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief A control block for one message limit.
 */
struct control_block_t
	{
		//! Limit value.
		unsigned int m_limit;

		//! The current count of the messages of that type.
		mutable std::atomic_uint m_count;

		//! Limit overflow reaction.
		action_t m_action;

		//! Initializing constructor.
		control_block_t(
			unsigned int limit,
			action_t action )
			:	m_limit( limit )
			,	m_action( std::move( action ) )
			{
				m_count = 0;
			}

		//! Copy constructor.
		control_block_t(
			const control_block_t & o )
			:	m_limit( o.m_limit )
			,	m_action( o.m_action )
			{
				m_count.store(
						o.m_count.load( std::memory_order_acquire ),
						std::memory_order_release );
			}

		//! Copy operator.
		control_block_t &
		operator=( const control_block_t & o )
			{
				m_limit = o.m_limit;
				m_count.store(
						o.m_count.load( std::memory_order_acquire ),
						std::memory_order_release );
				m_action = o.m_action;

				return *this;
			}

		//! A special indicator about absence of control_block.
		inline static const control_block_t *
		none() { return nullptr; }

		//! A safe decrement of message count with respect to absence of limit
		//! for a message.
		inline static void
		decrement( const control_block_t * limit )
			{
				if( limit )
					--(limit->m_count);
			}
	};

} /* namespace message_limit */

//
// upcastable_message_root_t
//
//FIXME: document this!
template< typename Derived >
class upcastable_message_root_t : public message_t
{
public:
	[[nodiscard]]
	std::optional<message_upcaster_t>
	so_direct_message_upcaster() const noexcept
	{
		return std::nullopt;
	}

	[[nodiscard]]
	std::optional<message_upcaster_t>
	so_message_upcaster() const noexcept = 0;
};

//
// upcastable_message_t
//
//FIXME: document this!
template< typename Derived, typename Base >
class upcastable_message_t : public message_t
{
	static_assert(
			std::is_base_of_v< upcastable_message_root_t<Base>, Base >,
			"Base should be derived from upcastable_message_root_t<Base>" );

private:
	static std::type_index
	so_base_type() noexcept
	{
		return typeid(Base);
	}

	static std::type_index
	so_actual_type() noexcept
	{
		return typeid(Derived);
	}

	static std::optional<message_upcaster_t>
	so_next_message_upcaster( const message_ref_t & ref ) noexcept
	{
		const auto * b = static_cast<const Base *>( ref.get() );
		return b->so_direct_message_upcaster();
	}

public:
	[[nodiscard]]
	std::optional<message_upcaster_t>
	so_direct_message_upcaster() const noexcept
	{
		return message_upcaster_t{
				&upcastable_message_t::so_base_type,
				&upcastable_message_t::so_actual_type,
				&upcastable_message_t::so_next_message_upcaster
		};
	}

	[[nodiscard]]
	std::optional<message_upcaster_t>
	so_message_upcaster() const noexcept override
	{
		return so_direct_message_upcaster();
	}
};

} /* namespace so_5 */

