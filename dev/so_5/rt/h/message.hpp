/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A base class for the agent messages definition.
*/

#pragma once

#include <so_5/h/declspec.hpp>
#include <so_5/h/exception.hpp>
#include <so_5/h/atomic_refcounted.hpp>
#include <so_5/h/types.hpp>

#include <so_5/rt/h/agent_ref_fwd.hpp>

#include <type_traits>
#include <typeindex>
#include <functional>
#include <future>
#include <atomic>

namespace so_5
{

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

		virtual ~message_t() SO_5_NOEXCEPT = default;

		/*!
		 * \brief Helper method for safe get of message mutability flag.
		 *
		 * Use this method instead of direct call to so5_message_mutability()
		 * because \a what will be nullptr for signals.
		 */
		friend message_mutability_t
		message_mutability( const intrusive_ptr_t<message_t> & what )
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
		message_mutability( const message_t & what )
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
		 * \since
		 * v.5.5.9
		 *
		 * \brief Get the pointer to the message payload.
		 *
		 * \note This method is necessary for message delivery tracing.
		 * For ordinal messages it will return a pointer to the message itself.
		 * For service requests and user-defined messages it will return
		 * pointer to a payload object.
		 */
		virtual const void *
		so5__payload_ptr() const;

		//FIXME: this method should be marked as noexcept in v.5.6.0
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
		so5_message_mutability() const { return m_mutability; }

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
		so5_message_kind() const SO_5_NOEXCEPT
			{
				return kind_t::classical_message;
			}
	};

//
// message_ref_t
//
//! A smart reference to the message.
/*!
 * \note Defined as typedef since v.5.2.0
 */
typedef intrusive_ptr_t< message_t > message_ref_t;

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
		so5_message_kind() const SO_5_NOEXCEPT override
			{
				return kind_t::classical_message;
			}

	public :
		~signal_t() SO_5_NOEXCEPT override = default;
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
		:	m_payload( T{ std::forward< Args >( args )... } )
		{}

	//! Initialization from const T object.
	user_type_message_t( const T & o )
		:	m_payload( o )
		{}

	//! Initialization from non-const T object.
	user_type_message_t( T & o )
		:	m_payload( o )
		{}

	//! Initialization from temporary T object.
	user_type_message_t( T && o )
		:	m_payload( std::move(o) )
		{}

private :
	virtual const void *
	so5__payload_ptr() const override { return &m_payload; }

	kind_t
	so5_message_kind() const SO_5_NOEXCEPT override
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

		static const SO_5_CONSTEXPR message_mutability_t mutability =
				message_mutability_t::immutable_message;
	};

template< typename T >
struct message_mutability_traits< immutable_msg<T> >
	{
		using payload_type = T;
		using subscription_type = T;
		using mhood_param_type = T;

		static const SO_5_CONSTEXPR message_mutability_t mutability =
				message_mutability_t::immutable_message;
	};

template< typename T >
struct message_mutability_traits< mutable_msg<T> >
	{
		using payload_type = T;
		using subscription_type = mutable_msg<T>;
		using mhood_param_type = mutable_msg<T>;

		static const SO_5_CONSTEXPR message_mutability_t mutability =
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
		static const bool is_signal = ::so_5::is_signal<T>::value;

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
		static const bool is_signal = false;

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

template< typename Msg >
typename std::enable_if<
	message_mutability_t::mutable_message !=
			::so_5::details::message_mutability_traits<Msg>::mutability >::type
mark_as_mutable_if_necessary( message_t & /*msg*/ )
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

//
// msg_service_request_base_t
//

/*!
 * \since
 * v.5.3.0
 *
 * \brief A base class for concrete messages with information
 * about service requests.
 */
class SO_5_TYPE msg_service_request_base_t : public message_t
{
	public:
		//! Setup exception information to underlying promise/future objects.
		virtual void
		set_exception( std::exception_ptr ex ) = 0;

		/*!
		 * \since
		 * v.5.5.5
		 *
		 * \brief Access to param of service_request.
		 */
		virtual message_t &
		query_param() const SO_5_NOEXCEPT = 0;

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Helper wrapper for handling exceptions during
		 * service request dispatching.
		 */
		template< class Lambda >
		static void
		dispatch_wrapper(
			const message_ref_t & what,
			Lambda handler )
		{
			try
			{
				handler();
			}
			catch( ... )
			{
				msg_service_request_base_t & svc_request =
						*(dynamic_cast< msg_service_request_base_t * >(
								what.get() ));

				svc_request.set_exception( std::current_exception() );
			}
		}

	private :
		// This method must be reimplemented in derived types.
		virtual message_mutability_t
		so5_message_mutability() const override = 0;

		// This method must be reimplemented in derived types.
		virtual void
		so5_change_mutability( message_mutability_t ) override = 0;

		kind_t
		so5_message_kind() const SO_5_NOEXCEPT override
			{
				return kind_t::service_request;
			}
};

//
// msg_service_request_t
//
/*!
 * \since
 * v.5.3.0
 *
 * \brief A concrete message with information about service request.
 */
template< class Result, class Param >
struct msg_service_request_t : public msg_service_request_base_t
	{
		//! A promise object for result of service function.
		std::promise< Result > m_promise;
		//! A parameter for service function.
		message_ref_t m_param;

		//! Constructor for the case where Param is a signal.
		msg_service_request_t(
			std::promise< Result > && promise )
			:	m_promise( std::move( promise ) )
			{}

		//! Constructor for the case where Param is a message.
		msg_service_request_t(
			std::promise< Result > && promise,
			message_ref_t && param )
			:	m_promise( std::move( promise ) )
			,	m_param( std::move( param ) )
			{}

		virtual void
		set_exception( std::exception_ptr what ) override
			{
				m_promise.set_exception( what );
			}

		virtual message_t &
		query_param() const SO_5_NOEXCEPT override
			{
				return *m_param;
			}

	private :
		virtual const void *
		so5__payload_ptr() const override { return m_param.get(); }

		virtual message_mutability_t
		so5_message_mutability() const override
			{
				return message_mutability( m_param );
			}

		virtual void
		so5_change_mutability( message_mutability_t v ) override
			{
				change_message_mutability( *m_param, v );
			}
	};

// Note: after introduction of message_kind_t this enumeration
// is seems to be obsolete. It will probably be removed in v.5.6.0
//
// invocation_type_t
//
/*!
 * \since
 * v.5.3.0
 *
 * \brief Type of agent method invocation (event handling, service request).
 */
enum class invocation_type_t : int
	{
		//! Ordinal event handler invocation.
		/*! Return value of event handler could be skipped. */
		event,
		//! Service handler invocation.
		/*!
		 * Return value of service handler should be stored into
		 * underlying std::promise object.
		 */
		service_request,
		//! Enveloped message.
		/*!
		 * This is a special envelope with some message/signal inside.
		 *
		 * \since
		 * v.5.5.23
		 */
		enveloped_msg
	};

//NOTE: this function should probably be removed in v.5.6.0.
/*!
 * \brief Detect invocation type by analyzing message_kind value.
 *
 * \since
 * v.5.5.23
 */
inline invocation_type_t
detect_invocation_type_for_message_kind(
	message_t::kind_t kind )
	{
		invocation_type_t result = invocation_type_t::event;
		switch( kind )
			{
			case message_t::kind_t::signal: /* event */ break;
			case message_t::kind_t::classical_message: /* event */ break;
			case message_t::kind_t::user_type_message: /* event */ break;

			case message_t::kind_t::service_request:
				result = invocation_type_t::service_request;
			break;

			case message_t::kind_t::enveloped_msg:
				result = invocation_type_t::enveloped_msg;
			break;
			}

		return result;
	}

/*!
 * \brief Detect invocation_type for a message.
 *
 * \since
 * v.5.5.23
 */
inline invocation_type_t
detect_invocation_type_for_message(
	const message_t & msg )
	{
		return detect_invocation_type_for_message_kind( message_kind(msg) );
	}

/*!
 * \brief Detect invocation_type for a message.
 *
 * \since
 * v.5.5.23
 */
inline invocation_type_t
detect_invocation_type_for_message(
	const message_ref_t & msg )
	{
		return detect_invocation_type_for_message_kind( message_kind(msg) );
	}

namespace message_limit
{

struct control_block_t;

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

		//! Is it message delivery or service request delivery.
		invocation_type_t m_event_type;

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
			invocation_type_t event_type,
			unsigned int reaction_deep,
			const std::type_index & msg_type,
			const message_ref_t & message,
			const impl::action_msg_tracer_t * msg_tracer )
			:	m_mbox_id( mbox_id )
			,	m_receiver( receiver )
			,	m_limit( limit )
			,	m_event_type( event_type )
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

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::message_t instead.
 */
using message_t = so_5::message_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::message_ref_t
 * instead.
 */
using message_ref_t = so_5::message_ref_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::signal_t
 * instead.
 */
using signal_t = so_5::signal_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::user_type_message_t
 * instead.
 */
template< typename T >
using user_type_message_t = so_5::user_type_message_t< T >;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::is_user_type_message
 * instead.
 */
template< typename M >
using is_user_type_message = so_5::is_user_type_message< M >;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::is_signal instead.
 */
template< class T >
using is_signal = so_5::is_signal< T >;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::is_classical_message
 * instead.
 */
template< class T >
using is_classical_message = so_5::is_classical_message< T >;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::ensure_not_signal
 * instead.
 */
template< class Msg >
void ensure_not_signal() { so_5::ensure_not_signal< Msg >(); }

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::ensure_message_with_actual_data instead.
 */
template< class Msg >
void ensure_message_with_actual_data( const Msg * m ) {
	so_5::ensure_message_with_actual_data( m );
}

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::ensure_signal instead.
 */
template< class Msg >
void ensure_signal() { so_5::ensure_signal< Msg >(); }

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::ensure_classical_message
 * instead.
 */
template< typename Msg >
void ensure_classical_message() {
	so_5::ensure_classical_message< Msg >();
}

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::message_payload_type
 * instead.
 */
template< typename T >
using message_payload_type = so_5::message_payload_type< T >;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::msg_service_request_base_t
 * instead.
 */
using msg_service_request_base_t = so_5::msg_service_request_base_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::msg_service_request_t
 * instead.
 */
template< class Result, class Param >
using msg_service_request_t = so_5::msg_service_request_t< Result, Param >;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::invocation_type_t
 * instead.
 */
using invocation_type_t = so_5::invocation_type_t;

namespace message_limit
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::message_limit::overlimit_context_t instead.
 */
using overlimit_context_t = so_5::message_limit::overlimit_context_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::message_limit::action_t
 * instead.
 */
using action_t = so_5::message_limit::action_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::message_limit::control_block_t instead.
 */
using control_block_t = so_5::message_limit::control_block_t;

} /* namespace message_limit */

} /* namespace rt */

} /* namespace so_5 */

