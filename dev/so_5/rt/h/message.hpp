/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A base class for the agent messages definition.
*/

#if !defined( _SO_5__RT__MESSAGE_HPP_ )
#define _SO_5__RT__MESSAGE_HPP_

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

namespace rt
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
class SO_5_TYPE message_t
	:
		public atomic_refcounted_t
{
	public:
		message_t();
		message_t( const message_t & );
		void
		operator = ( const message_t & );

		virtual ~message_t();
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
// is_message
//
/*!
 * \since v.5.5.4
 * \brief A helper class for checking that message is a message.
 */
template< class T >
struct is_message
	{
		enum { value = (std::is_base_of< message_t, T >::value &&
				!is_signal< T >::value) };
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
	static_assert( is_message< MSG >::value,
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

namespace details
{

template< bool is_signal, typename MSG >
struct make_message_instance_impl
	{
		template< typename... ARGS >
		static std::unique_ptr< MSG >
		make( ARGS &&... args )
			{
				ensure_not_signal< MSG >();

				return std::unique_ptr< MSG >(
						new MSG( std::forward< ARGS >(args)... ) );
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
std::unique_ptr< MSG >
make_message_instance( ARGS &&... args )
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

		//! Initializing constructor.
		inline
		overlimit_context_t(
			const agent_t & receiver,
			const control_block_t & limit,
			invocation_type_t event_type,
			unsigned int reaction_deep,
			const std::type_index & msg_type,
			const message_ref_t & message )
			:	m_receiver( receiver )
			,	m_limit( limit )
			,	m_event_type( event_type )
			,	m_reaction_deep( reaction_deep )
			,	m_msg_type( msg_type )
			,	m_message( message )
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

} /* namespace rt */

} /* namespace so_5 */

#endif

