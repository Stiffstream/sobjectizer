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

//FIXME: document this!
/*!
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
			//! Can the delivery blocks the current thread?
			message_delivery_mode_t delivery_mode,
			//! Type of the message to deliver.
			const std::type_index & msg_type,
			//! A message instance to be delivered.
			const message_ref_t & message,
			//! Current deep of overlimit reaction recursion.
			unsigned int redirection_deep ) override;

		/*!
		 * \note
		 * It throws because this mbox doesn't support delivery filters and can
		 * be used for message delivery only.
		 */
		void
		set_delivery_filter(
			//! Message type to be filtered.
			const std::type_index & msg_type,
			//! Filter to be set.
			//! A caller must guaranted the validity of this reference.
			const delivery_filter_t & filter,
			//! A subscriber for the message.
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
//FIXME: document this!
/*!
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
//FIXME: document this!
/*!
 * \since v.5.8.3
 */
[[nodiscard]]
inline so_5::mbox_t
make_special_enveloping_mbox( so_5::mbox_t dest )
	{
		return { std::make_unique< special_enveloping_mbox_t >( std::move(dest) ) };
	}

} /* namespace impl */

//
// individual
//
//FIXME: document this!
/*!
 * \since v.5.8.3
 */
template< typename Dest >
[[nodiscard]]
so_5::mbox_t
individual_trace( Dest && dest )
	{
		return impl::make_special_enveloping_mbox(
				so_5::send_functions_details::arg_to_mbox( std::forward<Dest>(dest) ) );
	}

//
// make_individual_trace_filter
//
//FIXME: document this!
[[nodiscard]] SO_5_FUNC
filter_shptr_t
make_individual_trace_filter();

} /* namespace so_5::msg_tracing */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

