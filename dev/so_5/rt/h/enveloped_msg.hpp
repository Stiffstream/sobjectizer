/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.23
 *
 * \brief Stuff related to enveloped messages.
 */

#pragma once

#include <so_5/rt/h/message.hpp>

#include <so_5/h/optional.hpp>

namespace so_5 {

namespace enveloped_msg {

//
// payload_info_t
//
/*!
 * \brief An information about payload inside envelope.
 *
 * This class is necessary for incapsulation of information related
 * to payload inside an envelope. In v.5.5.23 this class contains only
 * reference to the payload message. But additional info can be added
 * in some future versions.
 *
 * \since
 * v.5.5.23
 */
class payload_info_t
	{
		//! Actual enveloped message.
		/*!
		 * This member is mutable because we need to return non-const
		 * reference to message_ref_t from const method message().
		 *
		 * \note
		 * This pointer can be null if the enveloped message is a signal.
		 */
		mutable message_ref_t m_message;

	public:
		//! Initializing constructor.
		payload_info_t(
			message_ref_t message )
			:	m_message{ std::move(message) }
			{}

		/*!
		 * \name Getters
		 * \{
		 */
		SO_5_NODISCARD
		message_ref_t &
		message() const SO_5_NOEXCEPT { return m_message; }
		/*!
		 * \}
		 */
	};

//
// handler_invoker_t
//
/*!
 * \brief An extended version of handling_context which can be used
 * for calling event handler.
 *
 * \note
 * This class has non-virtual and protected destructor because
 * creation on instances derived classes in dynamic memory
 * is not intended.
 *
 * \since
 * v.5.5.23
 */
class SO_5_TYPE handler_invoker_t
	{
	protected:
		// clang requires this.
		handler_invoker_t() = default;
		handler_invoker_t( const handler_invoker_t & ) = default;
		handler_invoker_t & operator=( const handler_invoker_t & ) = default;

#if !defined(SO_5_MSVC_CANT_DEFAULT_MOVE_CONSTRUCTOR)
		handler_invoker_t( handler_invoker_t && ) = default;
		handler_invoker_t & operator=( handler_invoker_t && ) = default;
#endif

		~handler_invoker_t() = default;

	public:
		//! Call an actual handler for the enveloped message/signal.
		virtual void
		invoke( const payload_info_t & payload ) SO_5_NOEXCEPT = 0;
	};

//
// envelope_t
//
/*!
 * \brief An interface of envelope with some message/signal inside.
 *
 * SObjectizer v.5.5.23 introduced a new thing: enveloped messages.
 * It means that actual message/signal is placed into a special
 * container called 'envelope'. This envelope is delivered to all
 * receivers of the original message/signal. But before the calling
 * of event handler in a receiver the original message/signal (e.g. payload)
 * are extracted from envelope and passed to the event handler.
 *
 * This interface describes 'envelope' for such containers. All envelopes
 * should implement this interface.
 *
 * Method handler_found_hook() is called by SObjectizer when envelope
 * is delivered to a receiver and receiver is ready to handle a message
 * from envelope. Envelope should check the availability of the payload and,
 * if the payload is available for processing, should pass the payload info
 * to handler_invoker_t::invoke() method.
 *
 * Method transformation_hook() is called by SObjectizer when envelope can't
 * be delivered to a receiver in its current form and there is a need to
 * transform the message/signal from the envelope to another type
 * of message/signal. For example it can happen when limit_then_transform
 * is used for overload control. In that case the payload should be extracted
 * and passed to a transformation function.
 * 
 * When transformation_hook() is called the envelope should check the
 * availability of the payload and, if the payload is available for processing,
 * should pass the payload info to handler_invoker_t::invoke() method.
 *
 * Please note that call of handler_invoker_t::invoke() is not guaranteed.
 * Envelope can check some conditions (like payload expiration or revocation)
 * and does call to invoke() only if these conditions are meet. But if some
 * conditions are not fulfilled then handler_found_hook() and
 * transformation_hook() won't call handler_invoker_t::invoke() method.
 *
 * \since
 * v.5.5.23
 */
class SO_5_TYPE envelope_t : public message_t
	{
	public:
		// Introduce some names into the scope of this class to
		// be easily used in derived classes outside so_5 namespace.
		using payload_info_t = ::so_5::enveloped_msg::payload_info_t;
		using handler_invoker_t = ::so_5::enveloped_msg::handler_invoker_t;

		// clang requires this.
		envelope_t() = default;
		envelope_t( const envelope_t & ) = default;
		envelope_t & operator=( const envelope_t & ) = default;

#if !defined(SO_5_MSVC_CANT_DEFAULT_MOVE_CONSTRUCTOR)
		envelope_t( envelope_t && ) = default;
		envelope_t & operator=( envelope_t && ) = default;
#endif

		virtual ~envelope_t() override = default;

		//! Hook for the case when there is an event handler for 
		//! the enveloped message/signal for a particular receiver.
		virtual void
		handler_found_hook(
			//! Proxy object which can call an actual event handler.
			handler_invoker_t & invoker ) SO_5_NOEXCEPT = 0;

		//! Hook for the case when a message will be transformed to
		//! some other message object.
		virtual void
		transformation_hook(
			//! Proxy object which should receive an actual payload
			//! (or should not be called at all if there is no more
			//! payload inside the envelope).
			handler_invoker_t & invoker ) SO_5_NOEXCEPT = 0;

	private :
		kind_t
		so5_message_kind() const SO_5_NOEXCEPT override
			{
				return kind_t::enveloped_msg;
			}
	};

/*!
 * \brief Helper function for extraction of a payload
 * from enveloped message.
 *
 * Extraction of a payload from an envelope is not an easy task.
 * It is necessary to create an implementation of handler_invoker_t
 * interface and pass it to envelope_t::transformation_hook() method.
 * This implementation should check type of the payload: if it is
 * another envelope then next call to transformation_hook() should
 * be done and so on.
 *
 * Because of that extraction of the payload from an envelope is a boring
 * task. This helper function preforms all described actions an returns
 * optional with payload inside (if the payload available).
 *
 * \attention
 * Argument \a envelope should not be nullptr.
 *
 * \since
 * v.5.5.23
 */
SO_5_FUNC
SO_5_NODISCARD
optional< payload_info_t >
extract_payload_for_message_transformation(
	//! Envelope with message inside.
	const message_ref_t & envelope );
	
} /* namespace enveloped_msg */

} /* namespace so_5 */

