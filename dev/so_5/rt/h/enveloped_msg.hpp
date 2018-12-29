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
// access_context_t
//
/*!
 * \brief Information about context on that enveloped message is handled.
 *
 * \since
 * v.5.5.23
 */
enum class access_context_t
	{
		//! Enveloped message is delivered to a receiver and the payload
		//! is necessary for calling event handler.
		handler_found,
		//! The content of enveloped message should be transformed to
		//! another representation.
		//! For example it can be necessary for limit_then_transform
		//! overload reaction.
		transformation,
		//! The content of enveloped message should be analyzed for
		//! the further delivery.
		//! For example it can be necessary for delivery filters.
		inspection
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
 * Method access_hook() is called by SObjectizer when the payload
 * of enveloped message should be accessed. For example:
 *
 * - envelope is delivered to a receiver and receiver is ready to handle a
 *   message from envelope;
 * - envelope can't be delivered to a receiver in its current form and there is
 *   a need to transform the message/signal from the envelope to another type
 *   of message/signal. For example it can happen when limit_then_transform is
 *   used for overload control. In that case the payload should be extracted
 *   and passed to a transformation function;
 * - envelope should be analyzed by a delivery filter for further delivery
 *   of message.
 * 
 * When access_hook() is called the envelope should check the
 * availability of the payload and, if the payload is available for processing,
 * should pass the payload info to handler_invoker_t::invoke() method.
 *
 * Please note that call of handler_invoker_t::invoke() is not guaranteed.
 * Envelope can check some conditions (like payload expiration or revocation)
 * and does call to invoke() only if these conditions are meet. But if some
 * conditions are not fulfilled then access_hook() won't call
 * handler_invoker_t::invoke() method.
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
		using access_context_t = ::so_5::enveloped_msg::access_context_t;

		// clang requires this.
		envelope_t() = default;
		envelope_t( const envelope_t & ) = default;
		envelope_t & operator=( const envelope_t & ) = default;

#if !defined(SO_5_MSVC_CANT_DEFAULT_MOVE_CONSTRUCTOR)
		envelope_t( envelope_t && ) = default;
		envelope_t & operator=( envelope_t && ) = default;
#endif

		virtual ~envelope_t() override = default;

		virtual void
		access_hook(
			//! Why this hook is called.
			access_context_t context,
			//! Proxy object which can call an actual event handler.
			handler_invoker_t & invoker ) SO_5_NOEXCEPT = 0;

	private :
		kind_t
		so5_message_kind() const SO_5_NOEXCEPT override
			{
				return kind_t::enveloped_msg;
			}
	};

//
// extract_payload_for_message_transformation
//
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
SO_5_NODISCARD
SO_5_FUNC
optional< payload_info_t >
extract_payload_for_message_transformation(
	//! Envelope with message inside.
	const message_ref_t & envelope );

//
// message_to_be_inspected
//
/*!
 * \brief Helper function for extraction of a payload from enveloped
 * message.
 *
 * This function checks the kind of \a msg_or_envelope. If this is
 * an enveloped message message_to_be_inspected() will try to extract
 * the payload and return it. In that case an empty \a optional object
 * can be returned.
 *
 * If \a msg_or_envelope is not an envelope then \a msg_or_envelope
 * is returned as a result.
 *
 * \since
 * v.5.5.23
 */
SO_5_NODISCARD
SO_5_FUNC
optional< message_ref_t >
message_to_be_inspected(
	const message_ref_t & msg_or_envelope );
	
} /* namespace enveloped_msg */

} /* namespace so_5 */

