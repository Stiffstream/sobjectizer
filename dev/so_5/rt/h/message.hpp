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

#include <type_traits>
#include <typeindex>
#include <future>

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
typedef smart_atomic_reference_t< message_t > message_ref_t;

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
	static_assert( !std::is_base_of< signal_t, MSG >::value,
			"signal_t instance cannot be used in place of"
			" message_t instance" );
	static_assert( std::is_base_of< message_t, MSG >::value,
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
	static_assert( std::is_base_of< signal_t, MSG >::value,
			"expected a type derived from the signal_t" );
}

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
		set_exception( std::exception_ptr what )
			{
				m_promise.set_exception( what );
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

} /* namespace rt */

} /* namespace so_5 */

#endif

