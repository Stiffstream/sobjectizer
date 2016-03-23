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
 */
class SO_5_TYPE message_t : public atomic_refcounted_t
	{
		friend class impl::internal_message_iface_t;

	public:
		message_t();
		message_t( const message_t & );
		void
		operator = ( const message_t & );

		virtual ~message_t();

	private :
		/*!
		 * \since v.5.5.9
		 * \brief Get the pointer to the message payload.
		 *
		 * \note This method is necessary for message delivery tracing.
		 * For ordinal messages it will return a pointer to the message itself.
		 * For service requests and user-defined messages it will return
		 * pointer to a payload object.
		 */
		virtual const void *
		so5__payload_ptr() const;
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
 * \since v.5.2.0
 *
 * All signals (messages without any data) for agents should be
 * derived from this class.
 */
class SO_5_TYPE signal_t
	:	public message_t
{
	private :
		/*!
		 * Private constructor to disable creation of instances of
		 * derived classes.
		 */
		signal_t();

	public :
		virtual ~signal_t();
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
	const T m_payload;

	//! Initializing constructor.
	template< typename... ARGS >
	user_type_message_t( ARGS &&... args )
		:	m_payload( T{ std::forward< ARGS >( args )... } )
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
};

//
// is_user_type_message
//
/*!
 * \since v.5.5.9
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
 * \since v.5.5.4
 * \brief A helper class for checking that message is a signal.
 */
template< class T >
struct is_signal
	{
		enum { value = std::is_base_of< signal_t, T >::value };
	};

//
// is_classical_message
//
/*!
 * \since v.5.5.9
 * \brief A helper class for checking that message is a classical message
 * derived from %message_t class.
 */
template< class T >
struct is_classical_message
	{
		enum { value = std::is_base_of< message_t, T >::value };
	};

//
// ensure_not_signal
//
/*!
 * \since v.5.2.0
 * \brief A special compile-time checker to guarantee that the message
 * class is not a signal class.
 */
template< class MSG >
void
ensure_not_signal()
{
	static_assert( !is_signal< MSG >::value,
			"message class must be derived from the message_t" );
}

//
// ensure_message_with_actual_data
//
/*!
 * \since v.5.2.0
 * \brief A special checker to guarantee that the message is an instance
 * of the message_t (not signal_t) and has a not-null pointer 
 * to the message data.
 *
 * \note A check for the inheritance from the message_t is done at compile-time.
 *
 * \tparam MSG message type to be checked.
 */
template< class MSG >
void
ensure_message_with_actual_data( const MSG * m )
{
	ensure_not_signal< MSG >();

	if( !m )
		throw so_5::exception_t(
				"an attempt to send a message via nullptr",
				so_5::rc_null_message_data );
}

//
// ensure_signal
//
/*!
 * \since v.5.2.0
 * \brief A special compile-time checker to guarantee that the MSG is derived
 * from the signal_t.
 *
 * \tparam MSG signal type to be checked.
 */
template< class MSG >
void
ensure_signal()
{
	static_assert( is_signal< MSG >::value,
			"expected a type derived from the signal_t" );
}

//
// ensure_classical_message
//
/*!
 * \since v.5.5.9
 * \brief A special compile-time checker to guarantee that MSG is derived from
 * %message_t.
 *
 * \tparam MSG type to be checked.
 */
template< typename MSG >
void
ensure_classical_message()
	{
		static_assert( is_classical_message< MSG >::value,
				"expected a type derived from the message_t" );
	}

//
// message_payload_type_impl
//
/*!
 * \since v.5.5.9
 * \brief Implementation details for %message_payload_type.
 *
 * \note This specialization is for cases where T is derived from message_t.
 * In that case payload_type is the same as envelope_type.
 */
template< typename T, bool is_classical_message >
struct message_payload_type_impl
	{
		//! Type visible to user.
		using payload_type = T;
		//! Type for message delivery.
		using envelope_type = T;

		//! Type ID for subscription.
		inline static std::type_index payload_type_index() { return typeid(T); }

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
		inline static const payload_type &
		payload_reference( const message_t & msg )
			{
				return dynamic_cast< const payload_type & >( msg );
			}
	};

/*!
 * \since v.5.5.9
 * \brief Implementation details for %message_payload_type.
 *
 * \note This specialization is for cases where T is not derived from message_t.
 * In that case payload_type is T, but envelope_type is user_type_message_t<T>.
 */
template< typename T >
struct message_payload_type_impl< T, false >
	{
		//! Type visible to user.
		using payload_type = T;
		//! Type for message delivery.
		using envelope_type = user_type_message_t< T >;

		//! Type ID for subscription.
		inline static std::type_index payload_type_index() { return typeid(T); }

		//! Helper for extraction of pointer to payload part.
		/*!
		 * \note This method return const pointer because payload is
		 * a const object inside user_type_message_t<T> instance.
		 */
		inline static const payload_type *
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
		inline static const payload_type &
		payload_reference( const message_t & msg )
			{
				auto & envelope = dynamic_cast< const envelope_type & >( msg );
				return envelope.m_payload;
			}
	};

//
// message_payload_type
//
/*!
 * \since v.5.5.9
 * \brief A helper class for detection of payload type of message.
 *
 * \tparam T type to test.
 */
template< typename T >
struct message_payload_type
	:	public message_payload_type_impl< T, is_classical_message< T >::value >
	{
	};

template< typename T >
struct message_payload_type< user_type_message_t< T > >
	:	public message_payload_type_impl< T, false >
	{
	};

namespace details
{

template< bool is_signal, typename MSG >
struct make_message_instance_impl
	{
		using E = typename message_payload_type< MSG >::envelope_type;

		template< typename... ARGS >
		static std::unique_ptr< E >
		make( ARGS &&... args )
			{
				ensure_not_signal< MSG >();

				return std::unique_ptr< E >( new E( std::forward< ARGS >(args)... ) );
			}
	};

template< typename MSG >
struct make_message_instance_impl< true, MSG >
	{
		static std::unique_ptr< MSG >
		make()
			{
				ensure_signal< MSG >();

				return std::unique_ptr< MSG >();
			}
	};

/*!
 * \since v.5.5.4
 * \brief A helper for allocate instance of a message.
 */
template< typename MSG, typename... ARGS >
auto
make_message_instance( ARGS &&... args )
	-> std::unique_ptr< typename message_payload_type< MSG >::envelope_type >
	{
		return make_message_instance_impl<
						is_signal< MSG >::value, MSG
				>::make( std::forward< ARGS >( args )... );
	}

} /* namespace details */

//
// msg_service_request_base_t
//

/*!
 * \since v.5.3.0
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
		 * \since v.5.5.5
		 * \brief Access to param of service_request.
		 */
		virtual const message_t &
		query_param() const SO_5_NOEXCEPT = 0;

		/*!
		 * \since v.5.5.4
		 * \brief Helper wrapper for handling exceptions during
		 * service request dispatching.
		 */
		template< class LAMBDA >
		static void
		dispatch_wrapper(
			const message_ref_t & what,
			LAMBDA handler )
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
};

//
// msg_service_request_t
//
/*!
 * \since v.5.3.0
 * \brief A concrete message with information about service request.
 */
template< class RESULT, class PARAM >
struct msg_service_request_t : public msg_service_request_base_t
	{
		//! A promise object for result of service function.
		std::promise< RESULT > m_promise;
		//! A parameter for service function.
		message_ref_t m_param;

		//! Constructor for the case where PARAM is a signal.
		msg_service_request_t(
			std::promise< RESULT > && promise )
			:	m_promise( std::move( promise ) )
			{}

		//! Constructor for the case where PARAM is a message.
		msg_service_request_t(
			std::promise< RESULT > && promise,
			message_ref_t && param )
			:	m_promise( std::move( promise ) )
			,	m_param( std::move( param ) )
			{}

		virtual void
		set_exception( std::exception_ptr what ) override
			{
				m_promise.set_exception( what );
			}

		virtual const message_t &
		query_param() const SO_5_NOEXCEPT override
			{
				return *m_param;
			}

	private :
		virtual const void *
		so5__payload_ptr() const override { return m_param.get(); }
	};

//
// invocation_type_t
//
/*!
 * \since v.5.3.0
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
		service_request
	};

namespace message_limit
{

struct control_block_t;

namespace impl
{

class action_msg_tracer_t;

} /* namespace impl */

/*!
 * \since v.5.5.4
 * \brief Description of context for overlimit action.
 */
struct overlimit_context_t
	{
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
		 * \since v.5.5.9
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
			const agent_t & receiver,
			const control_block_t & limit,
			invocation_type_t event_type,
			unsigned int reaction_deep,
			const std::type_index & msg_type,
			const message_ref_t & message,
			const impl::action_msg_tracer_t * msg_tracer )
			:	m_receiver( receiver )
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
 * \since v.5.5.4
 * \brief A type for reaction of message overlimit.
 */
using action_t = std::function< void(const overlimit_context_t&) >;

//
// control_block_t
//
/*!
 * \since v.5.5.4
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
template< class MSG >
void ensure_not_signal() { so_5::ensure_not_signal< MSG >(); }

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::ensure_message_with_actual_data instead.
 */
template< class MSG >
void ensure_message_with_actual_data( const MSG * m ) {
	so_5::ensure_message_with_actual_data( m );
}

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::ensure_signal instead.
 */
template< class MSG >
void ensure_signal() { so_5::ensure_signal< MSG >(); }

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::ensure_classical_message
 * instead.
 */
template< typename MSG >
void ensure_classical_message() {
	so_5::ensure_classical_message< MSG >();
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
template< class RESULT, class PARAM >
using msg_service_request_t = so_5::msg_service_request_t< RESULT, PARAM >;

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

