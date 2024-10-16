/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Stuff related to individual message delivery tracing.
 *
 * \since v.5.8.3
 */

#pragma once

#include <so_5/msg_tracing.hpp>

#include <so_5/enveloped_msg.hpp>
#include <so_5/mbox.hpp>
#include <so_5/send_functions.hpp>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5::msg_tracing
{

namespace impl
{

/*!
 * \brief A special mbox that wraps all incoming messages/signals into a
 * special envelope.
 *
 * Instances of that special mbox will be created by
 * so_5::msg_tracing::individual_trace().
 *
 * This mbox wraps outgoing messages/signals into an envelope of
 * type individual_tracing_envelope_t.
 *
 * \note
 * This mbox works like a simple proxy and because of that doesn't support
 * operations like subscriptions and delivery filter management.
 *
 * \since v.5.8.3
 */
class SO_5_TYPE special_enveloping_mbox_t final : public abstract_message_box_t
	{
		//! The real destination for the messages/signals.
		const mbox_t m_dest;

	public:
		//! Initializing constructor.
		special_enveloping_mbox_t(
			//! The real destination for the messages/signals.
			//! It's assumed that this pointer can't be nullptr.
			mbox_t dest );

		/*!
		 * \note
		 * Returns the ID of the actual destination.
		 */
		[[nodiscard]]
		mbox_id_t
		id() const override;

		/*!
		 * \note
		 * It throws because this mbox doesn't support subscriptions and can
		 * be used for message delivery only.
		 */
		void
		subscribe_event_handler(
			const std::type_index & type_index,
			abstract_message_sink_t & subscriber ) override;

		/*!
		 * \note
		 * Does nothing because this mbox doesn't support subscriptions and can
		 * be used for message delivery only.
		 */
		void
		unsubscribe_event_handler(
			const std::type_index & type_index,
			abstract_message_sink_t & subscriber ) noexcept override;

		/*!
		 * \note
		 * Returns the name of the actual destination mbox.
		 */
		[[nodiscard]]
		std::string
		query_name() const override;

		/*!
		 * \note
		 * Returns the type of the actual destination mbox.
		 */
		[[nodiscard]]
		mbox_type_t
		type() const override;

		/*!
		 * \brief Implementation of message delivery.
		 *
		 * Wraps the original message/signal into a special envelope and
		 * calls do_deliver_message on the actual destination mbox.
		 */
		void
		do_deliver_message(
			message_delivery_mode_t delivery_mode,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int redirection_deep ) override;

		/*!
		 * \note
		 * It throws because this mbox doesn't support delivery filters and can
		 * be used for message delivery only.
		 */
		void
		set_delivery_filter(
			const std::type_index & msg_type,
			const delivery_filter_t & filter,
			abstract_message_sink_t & subscriber ) override;

		/*!
		 * \note
		 * Does nothing because this mbox doesn't support delivery filters and can
		 * be used for message delivery only.
		 */
		void
		drop_delivery_filter(
			const std::type_index & msg_type,
			abstract_message_sink_t & subscriber ) noexcept override;

		[[nodiscard]]
		so_5::environment_t &
		environment() const noexcept override;
	};

//
// individual_tracing_envelope_t
//
/*!
 * \brief Special envelope that just holds an original message/signal.
 *
 * This envelope has no own logic. It returns the original message/signal
 * always.  The only purpose of this type is an attempt to make dynamic_cast in
 * a filter created by so_5::msg_tracing::make_individual_trace_filter(). This
 * filter tries to perform such a cast, and if this cast succeeds, then the
 * filter allows the trace.
 *
 * \since v.5.8.3
 */
class SO_5_TYPE individual_tracing_envelope_t final
	: public so_5::enveloped_msg::envelope_t
	{
		//! The original message/signal.
		const message_ref_t m_message;

	public:
		//! Initializing constructor.
		explicit individual_tracing_envelope_t(
			//! The original message/signal.
			message_ref_t message );

		void
		access_hook(
			access_context_t context,
			handler_invoker_t & invoker ) noexcept override;
	};

//
// make_special_enveloping_mbox
//
/*!
 * \brief Factory function for special_enveloping_mbox.
 *
 * Creates an instance of special_enveloping_mbox_t bound with \a dest and
 * returns it as mbox.
 *
 * \since v.5.8.3
 */
[[nodiscard]]
inline so_5::mbox_t
make_special_enveloping_mbox(
	//! Actual destination. It's assumed that \a dest != nullptr.
	so_5::mbox_t dest )
	{
		return { std::make_unique< special_enveloping_mbox_t >( std::move(dest) ) };
	}

} /* namespace impl */

//
// individual
//
/*!
 * \brief Indicator that tells that delivery of the message/signal should be traced.
 *
 * This indicator has to be used with send(), send_delayed() and send_periodic() functions.
 * It allows to trace only actions related to a message/signal sent by using
 * individual_trace().
 *
 * \note
 * It's necessary to enable message delivery tracing and set a filter returned
 * by make_individual_trace_filter().
 *
 * Usage example:
 * \code
 * // Ordinary message/signal.
 * so_5::send<my_msg>(so_5::msg_tracing::individual_trace(dest), ...);
 *
 * // Delayed message/signal.
 * so_5::send_delayed<my_msg>(so_5::msg_tracing::individual_trace(dest), 10ms, ...);
 *
 * // Periodic message/signal.
 * auto timer_id = so_5::send_periodic<my_msg>(so_5::msg_tracing::individual_trace(dest), 10ms, 10ms, ...);
 * \endcode
 *
 * \since v.5.8.3
 */
template< typename Dest >
[[nodiscard]]
so_5::mbox_t
individual_trace(
	//! Actual destination for the message/signal.
	//! It can be a reference to the agent (direct mbox of that agent will be used),
	//! a mbox, a mchain.
	Dest && dest )
	{
		return impl::make_special_enveloping_mbox(
				so_5::send_functions_details::arg_to_mbox( std::forward<Dest>(dest) ) );
	}

//
// make_individual_trace_filter
//
/*!
 * \brief Factory for special message tracing filter for individual tracing.
 *
 * This filter allows to trace only actions related to messages/signal sent by
 * using individual_trace().
 *
 * Usage example:
 * \code
 * so_5::launch(
 * 	[](so_5::environment_t & env) {...},
 * 	[](so_5::environment_params_t & params) {
 * 		// Message delivery tracing has to be turned on.
 * 		params.message_delivery_tracer(so_5::msg_tracing::std_cout_tracer());
 * 		// Special filter has to be set to avoid flooding of trace information.
 * 		params.message_delivery_tracer_filter(so_5::msg_tracing::make_individual_trace_filter());
 * 		...
 * 	});
 * \endcode
 *
 * \since v.5.8.3
 */
[[nodiscard]] SO_5_FUNC
filter_shptr_t
make_individual_trace_filter();

} /* namespace so_5::msg_tracing */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

