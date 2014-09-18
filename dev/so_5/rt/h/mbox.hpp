/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Mbox definition.
*/


#if !defined( _SO_5__RT__MBOX_HPP_ )
#define _SO_5__RT__MBOX_HPP_

#include <string>
#include <memory>
#include <typeindex>
#include <utility>

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>

#include <so_5/h/exception.hpp>

#include <so_5/rt/h/atomic_refcounted.hpp>
#include <so_5/rt/h/message.hpp>
#include <so_5/rt/h/event_data.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

class message_consumer_link_t;
class named_local_mbox_t;

} /* namespace impl */

class mbox_t;
class agent_t;

//
// mbox_ref_t
//
//! Smart reference for the mbox_t.
/*!
 * \note Defined as typedef since v.5.2.0
 */
typedef smart_atomic_reference_t< mbox_t > mbox_ref_t;

template< class RESULT >
class service_invoke_proxy_t;

/*!
 * \since v.5.3.0
 * \brief A special helper class for infinite waiting of service call.
 */
template< class RESULT >
class infinite_wait_service_invoke_proxy_t
	{
		//! Type of creator of that object.
		typedef service_invoke_proxy_t< RESULT > creator_t;

	public :
		infinite_wait_service_invoke_proxy_t(
			const creator_t & creator );

		//! Make synchronous service request call.
		/*!
		 * This method should be used for the case where PARAM is a signal.
		 */
		template< class PARAM >
		RESULT
		sync_get() const;

		//! Make synchronous service request call with parameter.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 */
		template< class PARAM >
		RESULT
		sync_get( smart_atomic_reference_t< PARAM > msg_ref ) const;

		//! Make synchronous service request call with parameter.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 */
		template< class PARAM >
		RESULT
		sync_get( std::unique_ptr< PARAM > msg_unique_ptr ) const;

		//! Make synchronous service request call with parameter.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 */
		template< class PARAM >
		RESULT
		sync_get( PARAM * msg ) const;

#if !defined( SO_5_NO_VARIADIC_TEMPLATES )
		//! Create param and make service request call.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 */
		template< class PARAM, typename... ARGS >
		RESULT
		make_sync_get( ARGS&&... args ) const;
#endif

	private :
		const creator_t m_creator;
	};

/*!
 * \since v.5.3.0
 * \brief A special helper class for waiting of service call for
 * the specified timeout.
 */
template< class RESULT, class DURATION >
class wait_for_service_invoke_proxy_t
	{
		//! Type of creator of that object.
		typedef service_invoke_proxy_t< RESULT > creator_t;

	public :
		wait_for_service_invoke_proxy_t(
			const creator_t & creator,
			const DURATION & timeout );

		//! Make synchronous service request call with parameter and
		//! wait timeout.
		/*!
		 * This method should be used for the case where PARAM is a signal.
		 *
		 * \throw so_5::exception_t with error code
		 * so_5::rc_svc_result_not_received_yet if there is no svc_handler result
		 * after timeout.
		 */
		template< class PARAM >
		RESULT
		sync_get() const;

		//! Make synchronous service request call with parameter and
		//! wait timeout.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 *
		 * \throw so_5::exception_t with error code
		 * so_5::rc_svc_result_not_received_yet if there is no svc_handler result
		 * after timeout.
		 */
		template< class PARAM >
		RESULT
		sync_get( smart_atomic_reference_t< PARAM > msg_ref ) const;

		//! Make synchronous service request call with parameter and
		//! wait timeout.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 *
		 * \throw so_5::exception_t with error code
		 * so_5::rc_svc_result_not_received_yet if there is no svc_handler result
		 * after timeout.
		 */
		template< class PARAM >
		RESULT
		sync_get( std::unique_ptr< PARAM > msg_unique_ptr ) const;

		//! Make synchronous service request call with parameter and
		//! wait timeout.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 *
		 * \throw so_5::exception_t with error code
		 * so_5::rc_svc_result_not_received_yet if there is no svc_handler result
		 * after timeout.
		 */
		template< class PARAM >
		RESULT
		sync_get( PARAM * msg ) const;

#if !defined( SO_5_NO_VARIADIC_TEMPLATES )
		//! Create param and make service request call.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 */
		template< class PARAM, typename... ARGS >
		RESULT
		make_sync_get( ARGS&&... args ) const;
#endif

	private :
		const creator_t m_creator;
		const DURATION m_timeout;
	};

/*!
 * \since v.5.3.0
 * \brief A special proxy for service request invocation.
 */
template< class RESULT >
class service_invoke_proxy_t
	{
	public :
		service_invoke_proxy_t( const mbox_ref_t & mbox );
		service_invoke_proxy_t( mbox_ref_t && mbox );

		//! Make asynchronous service request.
		/*!
		 * This method should be used for the cases where PARAM is a signal.
		 */
		template< class PARAM >
		std::future< RESULT >
		async() const;

		//! Make service request call with param.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 */
		template< class PARAM >
		std::future< RESULT >
		async( smart_atomic_reference_t< PARAM > msg_ref ) const;

		//! Make service request call with param.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 */
		template< class PARAM >
		std::future< RESULT >
		async( std::unique_ptr< PARAM > msg_unique_ptr ) const;

		//! Make service request call with param.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 */
		template< class PARAM >
		std::future< RESULT >
		async( PARAM * msg ) const;

		//! Make another proxy for time-unlimited synchronous
		//! service request calls.
		infinite_wait_service_invoke_proxy_t< RESULT >
		wait_forever() const;

		//! Make another proxy for time-limited synchronous
		//! service requests calls.
		template< class DURATION >
		wait_for_service_invoke_proxy_t< RESULT, DURATION >
		wait_for(
			//! Timeout for std::future::wait_for().
			const DURATION & timeout ) const;

#if !defined( SO_5_NO_VARIADIC_TEMPLATES )
		//! Create param and make service request call.
		/*!
		 * This method should be used for the case where PARAM is a message.
		 */
		template< class PARAM, typename... ARGS >
		std::future< RESULT >
		make_async( ARGS&&... args ) const;
#endif

	private :
		const mbox_ref_t m_mbox;
	};

//
// mbox_t
//

//! Mail box class.
/*!
 * The class serves as an interface for sending and receiving messages.
 *
 * All mboxes can be created via the SObjectizer Environment. References to
 * mboxes are stored and manipulated by so_5::rt::mbox_ref_t objects.
 *
 * mbox_t has two versions of the deliver_message() method. 
 * The first one requires pointer to the actual message data and is intended 
 * for delivering messages to agents.
 * The second one doesn't use a pointer to the actual message data and 
 * is intended for delivering signals to agents.
 *
 * mbox_t also is used for the delivery of delayed and periodic messages.
 * The SObjectizer Environment stores mbox for which messages must be
 * delivered and the timer thread pushes message instances to the mbox
 * at the appropriate time.
 *
 * \see so_environment_t::schedule_timer(), so_environment_t::single_timer().
 */
class SO_5_TYPE mbox_t
	:
		private atomic_refcounted_t
{
		friend class smart_atomic_reference_t< mbox_t >;

		mbox_t( const mbox_t & );
		void
		operator = ( const mbox_t & );

	public:
		mbox_t();
		virtual ~mbox_t();

		/*!
		 * \since v.5.4.0
		 * \brief Unique ID of this mbox.
		 */
		virtual mbox_id_t
		id() const = 0;

		//! Deliver message.
		/*!
		 * \since v.5.2.2
		 *
		 * Mbox takes care about destroying a message object.
		 */
		template< class MESSAGE >
		inline void
		deliver_message(
			//! Message data.
			const smart_atomic_reference_t< MESSAGE > & msg_ref ) const;

		//! Deliver message.
		/*!
		 * Mbox takes care about destroying a message object.
		 */
		template< class MESSAGE >
		inline void
		deliver_message(
			//! Message data.
			std::unique_ptr< MESSAGE > msg_unique_ptr ) const;

		//! Deliver message.
		/*!
		 * Mbox takes care about destroying a message object.
		 */
		template< class MESSAGE >
		inline void
		deliver_message(
			//! Message data.
			MESSAGE * msg_raw_ptr ) const;

		//! Deliver signal.
		template< class MESSAGE >
		inline void
		deliver_signal() const;

		/*!
		 * \since v.5.3.0
		 * \brief Create a special proxy for service request invocation.
		 */
		template< class RESULT >
		inline service_invoke_proxy_t< RESULT >
		get_one()
			{
				return service_invoke_proxy_t< RESULT >(
						mbox_ref_t( this ) );
			}

		/*!
		 * \since v.5.3.0
		 * \brief Create a special proxy for service request invocation
		 * where return type is void.
		 */
		inline service_invoke_proxy_t< void >
		run_one()
			{
				return service_invoke_proxy_t< void >( mbox_ref_t( this ) );
			}

		//! Deliver message for all subscribers.
		/*!
		 * \note This method is public since v.5.4.0.
		 */
		virtual void
		deliver_message(
			const std::type_index & type_index,
			const message_ref_t & message_ref ) const = 0;

		/*!
		 * \since v.5.3.0.
		 * \brief Deliver service request.
		 */
		virtual void
		deliver_service_request(
			//! This is type_index for service PARAM type.
			const std::type_index & type_index,
			//! This is reference to msg_service_request_t<RESULT,PARAM> instance.
			const message_ref_t & svc_request_ref ) const = 0;

		//! Add the message handler.
		virtual void
		subscribe_event_handler(
			//! Message type.
			const std::type_index & type_index,
			//! Agent-subcriber.
			agent_t * subscriber ) = 0;

		//! Remove all message handlers.
		virtual void
		unsubscribe_event_handlers(
			//! Message type.
			const std::type_index & type_index,
			//! Agent-subcriber.
			agent_t * subscriber ) = 0;

		//! Get the mbox name.
		virtual std::string
		query_name() const = 0;

		/*!
		 * \name Comparision.
		 * \{
		 */
		bool operator==( const mbox_t & o ) const;

		bool operator<( const mbox_t & o ) const;
		/*!
		 * \}
		 */
};

template< class MESSAGE >
inline void
mbox_t::deliver_message(
	const smart_atomic_reference_t< MESSAGE > & msg_ref ) const
{
	ensure_message_with_actual_data( msg_ref.get() );

	deliver_message(
		std::type_index( typeid( MESSAGE ) ),
		msg_ref.template make_reference< message_t >() );
}

template< class MESSAGE >
void
mbox_t::deliver_message(
	std::unique_ptr< MESSAGE > msg_unique_ptr ) const
{
	ensure_message_with_actual_data( msg_unique_ptr.get() );

	deliver_message(
		std::type_index( typeid( MESSAGE ) ),
		message_ref_t( msg_unique_ptr.release() ) );
}

template< class MESSAGE >
void
mbox_t::deliver_message(
	MESSAGE * msg_raw_ptr ) const
{
	this->deliver_message( std::unique_ptr< MESSAGE >( msg_raw_ptr ) );
}

template< class MESSAGE >
void
mbox_t::deliver_signal() const
{
	ensure_signal< MESSAGE >();

	deliver_message(
		std::type_index( typeid( MESSAGE ) ),
		message_ref_t() );
}

//
// service_invoke_proxy_t implementation.
//
template< class RESULT >
service_invoke_proxy_t<RESULT>::service_invoke_proxy_t(
	const mbox_ref_t & mbox )
	:	m_mbox( mbox )
	{}

template< class RESULT >
service_invoke_proxy_t<RESULT>::service_invoke_proxy_t(
	mbox_ref_t && mbox )
	:	m_mbox( std::move(mbox) )
	{}

template< class RESULT >
template< class PARAM >
std::future< RESULT >
service_invoke_proxy_t<RESULT>::async() const
	{
		ensure_signal< PARAM >();

		std::promise< RESULT > promise;
		auto f = promise.get_future();

		message_ref_t ref(
				new msg_service_request_t< RESULT, PARAM >(
						std::move(promise) ) );
		m_mbox->deliver_service_request(
				std::type_index( typeid(PARAM) ),
				ref );

		return f;
	}

template< class RESULT >
template< class PARAM >
std::future< RESULT >
service_invoke_proxy_t<RESULT>::async(
	smart_atomic_reference_t< PARAM > msg_ref ) const
	{
		ensure_message_with_actual_data( msg_ref.get() );

		std::promise< RESULT > promise;
		auto f = promise.get_future();

		message_ref_t ref(
				new msg_service_request_t< RESULT, PARAM >(
						std::move(promise),
						msg_ref.template make_reference< message_t >() ) );

		m_mbox->deliver_service_request(
				std::type_index( typeid(PARAM) ),
				ref );

		return f;
	}

template< class RESULT >
template< class PARAM >
std::future< RESULT >
service_invoke_proxy_t<RESULT>::async(
	std::unique_ptr< PARAM > msg_unique_ptr ) const
	{
		return this->async( smart_atomic_reference_t< PARAM >(
					msg_unique_ptr.release() ) );
	}

template< class RESULT >
template< class PARAM >
std::future< RESULT >
service_invoke_proxy_t<RESULT>::async( PARAM * msg ) const
	{
		return this->async( smart_atomic_reference_t< PARAM >( msg ) );
	}

template< class RESULT >
infinite_wait_service_invoke_proxy_t< RESULT >
service_invoke_proxy_t<RESULT>::wait_forever() const
	{
		return infinite_wait_service_invoke_proxy_t< RESULT >( *this );
	}

template< class RESULT >
template< class DURATION >
wait_for_service_invoke_proxy_t< RESULT, DURATION >
service_invoke_proxy_t<RESULT>::wait_for(
	const DURATION & timeout ) const
	{
		return wait_for_service_invoke_proxy_t< RESULT, DURATION >(
				*this, timeout );
	}

#if !defined( SO_5_NO_VARIADIC_TEMPLATES )
template< class RESULT >
template< class PARAM, typename... ARGS >
std::future< RESULT >
service_invoke_proxy_t<RESULT>::make_async( ARGS&&... args ) const
	{
		smart_atomic_reference_t< PARAM > msg(
				new PARAM( std::forward<ARGS>(args)... ) );

		return this->async( std::move( msg ) );
	}
#endif

//
// implementation of infinite_wait_service_invoke_proxy_t
//

template< class RESULT >
infinite_wait_service_invoke_proxy_t< RESULT >::
infinite_wait_service_invoke_proxy_t(
	const creator_t & creator )
	:	m_creator( creator )
	{}

template< class RESULT >
template< class PARAM >
RESULT
infinite_wait_service_invoke_proxy_t< RESULT >::sync_get() const
	{
		return m_creator.template async< PARAM >().get();
	}

template< class RESULT >
template< class PARAM >
RESULT
infinite_wait_service_invoke_proxy_t< RESULT >::sync_get(
	smart_atomic_reference_t< PARAM > msg_ref ) const
	{
		return m_creator.async( std::move(msg_ref) ).get();
	}

template< class RESULT >
template< class PARAM >
RESULT
infinite_wait_service_invoke_proxy_t< RESULT >::sync_get(
	std::unique_ptr< PARAM > msg_unique_ptr ) const
	{
		return this->sync_get(
				smart_atomic_reference_t< PARAM >( msg_unique_ptr.release() ) );
	}

template< class RESULT >
template< class PARAM >
RESULT
infinite_wait_service_invoke_proxy_t< RESULT >::sync_get( PARAM * msg ) const
	{
		return this->sync_get(
				smart_atomic_reference_t< PARAM >( msg ) );
	}

#if !defined( SO_5_NO_VARIADIC_TEMPLATES )
template< class RESULT >
template< class PARAM, typename... ARGS >
RESULT
infinite_wait_service_invoke_proxy_t< RESULT >::make_sync_get(
	ARGS&&... args ) const
	{
		return m_creator.template make_async< PARAM, ARGS... >(
				std::forward<ARGS>(args)... ).get();
	}
#endif /* SO_5_NO_VARIADIC_TEMPLATES */

//
// implemetation of wait_for_service_invoke_proxy_t
//
template< class RESULT, class DURATION >
wait_for_service_invoke_proxy_t< RESULT, DURATION >::wait_for_service_invoke_proxy_t(
	const creator_t & creator,
	const DURATION & timeout )
	:	m_creator( creator )
	,	m_timeout( timeout )
	{}

namespace wait_for_service_invoke_proxy_details
{
	template< class RESULT, class DURATION, class FUTURE >
	RESULT
	wait_and_return( const DURATION & timeout, FUTURE & f )
		{
#if defined( SO_5_STD_FUTURE_WAIT_FOR_ALWAYS_DEFFERED )
	#pragma message("THERE IS ERROR IN MSVC 11.0: std::future::wait_for returns std::future_status::deffered almost always")
#endif
			auto wait_result = f.wait_for( timeout );
			if( std::future_status::ready != wait_result )
				SO_5_THROW_EXCEPTION(
						rc_svc_result_not_received_yet,
						"no result from svc_handler after timeout" );
			
			return f.get();
		}

} /* namespace wait_for_service_invoke_proxy_details */

template< class RESULT, class DURATION >
template< class PARAM >
RESULT
wait_for_service_invoke_proxy_t< RESULT, DURATION >::sync_get() const
	{
		auto f = m_creator.template async< PARAM >();

		return wait_for_service_invoke_proxy_details::wait_and_return
				<RESULT, DURATION, decltype(f) >( m_timeout, f );
	}

template< class RESULT, class DURATION >
template< class PARAM >
RESULT
wait_for_service_invoke_proxy_t< RESULT, DURATION >::sync_get(
	smart_atomic_reference_t< PARAM > msg_ref ) const
	{
		auto f = m_creator.async( std::move(msg_ref) );

		return wait_for_service_invoke_proxy_details::wait_and_return
				<RESULT, DURATION, decltype(f) >( m_timeout, f );
		
		return f.get();
	}

template< class RESULT, class DURATION >
template< class PARAM >
RESULT
wait_for_service_invoke_proxy_t< RESULT, DURATION >::sync_get(
	std::unique_ptr< PARAM > msg_unique_ptr ) const
	{
		return this->sync_get(
				smart_atomic_reference_t< PARAM >(
						msg_unique_ptr.release() ) );
	}

template< class RESULT, class DURATION >
template< class PARAM >
RESULT
wait_for_service_invoke_proxy_t< RESULT, DURATION >::sync_get(
	PARAM * msg ) const
	{
		return this->sync_get(
				smart_atomic_reference_t< PARAM >( msg ) );
	}

#if !defined( SO_5_NO_VARIADIC_TEMPLATES )
template< class RESULT, class DURATION >
template< class PARAM, typename... ARGS >
RESULT
wait_for_service_invoke_proxy_t< RESULT, DURATION >::make_sync_get(
	ARGS&&... args ) const
	{
		smart_atomic_reference_t< PARAM > msg(
				new PARAM( std::forward<ARGS>(args)... ) );

		return this->sync_get( std::move( msg ) );
	}
#endif

} /* namespace rt */

} /* namespace so_5 */

#endif
