/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Stuff related to template type transformed_message.
 *
 * \since v.5.8.1
 */

#pragma once

#include <so_5/message.hpp>
#include <so_5/message_holder.hpp>
#include <so_5/mbox.hpp>

namespace so_5
{

namespace impl::transformed_message_impl
{

//
// message_maker
//
/*!
 * \brief A helper function to call appropriate constructor of
 * resulting type in dependence of message or signal.
 *
 * \tparam is_signal true if Msg is a signal.
 * \tparam Result Result type object of that will be created by make() method.
 * \tparam Msg Type of message to be instantiated.
 * \tparam Args Type of parameters to message constructor.
 *
 * \since v.5.5.4, v.5.8.1
 */
template< bool is_signal, typename Result, typename Msg, typename... Args >
struct message_maker
	{
		//! Create a message instance.
		static Result
		make( mbox_t mbox, Args &&... args )
			{
				return Result( std::move( mbox ),
						so_5::details::make_message_instance< Msg >(
								std::forward<Args>( args )... ) );
			}
	};

/*!
 * \brief A specialization of signal.
 *
 * \since v.5.5.4, v.5.8.1
 */
template< typename Result, typename Msg >
struct message_maker< true, Result, Msg >
	{
		static Result
		make( mbox_t mbox )
			{
				return Result( std::move( mbox ) );
			}
	};

} /* namespace impl::transformed_message_impl */

//
// transformed_message_t
//
/*!
 * \brief A result of message transformation.
 *
 * This template class is intended to store the result of
 * message transformation and redirection procedure performed by
 * so_5::agent_t::limit_then_transform, so_5::bind_transformer and
 * so_5::msinks::transform_then_redirect.
 *
 * An instance of this class is usually created by the static member
 * function make() or by a free function so_5::make_transformed.
 *
 * \tparam Msg Type of result (transformed) message. If `Msg` is the
 * message type, then `Msg`, `so_5::immutable_msg<Msg>` or
 * `so_5::mutable_msg<Msg>` can be used. If `Msg` is the signal type then
 * `Msg` or `so_5::immutable_msg<Msg>` is allowed.
 *
 * \since v.5.8.1
 */
template< typename Msg >
class transformed_message_t
	{
	public :
		//! Initializing constructor for the case when Msg is a message type.
		transformed_message_t(
			//! Message box to which transformed message to be sent.
			mbox_t mbox,
			//! New message instance.
			std::unique_ptr< typename message_payload_type< Msg >::envelope_type > msg )
			:	m_mbox( std::move( mbox ) )
			{
				ensure_message_with_actual_data( msg.get() );

				m_message = message_ref_t( msg.release() );
			}
		//! Initializing constructor for the case when Msg is a signal type.
		transformed_message_t(
			//! Message box to which signal to be sent.
			mbox_t mbox )
			:	m_mbox( std::move( mbox ) )
			{
				ensure_signal< Msg >();
			}

		//! Initializing constructor for the case when Msg is already
		//! exists as a %message_holder_t instance.
		/*!
		 * \since v.5.8.1
		 */
		template<
			typename Msg_In_Holder,
			message_ownership_t Ownership >
		transformed_message_t(
			mbox_t mbox,
			message_holder_t< Msg_In_Holder, Ownership > msg_holder )
			:	m_mbox{ std::move(mbox) }
			,	m_message{ msg_holder.make_reference() }
			{
				static_assert(
						std::is_same_v<
								typename message_payload_type< Msg >::envelope_type,
								typename message_payload_type< Msg_In_Holder >::envelope_type >,
						"message_holder has to hold a message of the same type" );
			}

		//! Destination message box.
		const mbox_t &
		mbox() const { return m_mbox; }

		//! Type of the transformed message.
		std::type_index
		msg_type() const { return message_payload_type< Msg >::subscription_type_index(); }

		//! Instance of transformed message.
		/*!
		 * \note Will be nullptr for signal.
		 */
		const message_ref_t &
		message() const { return m_message; }

		//! A helper method for transformed_message construction.
		template< typename... Args >
		static transformed_message_t< Msg >
		make( mbox_t mbox, Args &&... args )
			{
				return impl::transformed_message_impl::message_maker<
							is_signal< Msg >::value,
							transformed_message_t,
							Msg,
							Args...
						>::make(
								std::move( mbox ),
								std::forward<Args>( args )... );
			}

	private :
		//! Destination message box.
		mbox_t m_mbox;

		//! Instance of transformed message.
		/*!
		 * \note Will be nullptr for signal.
		 */
		message_ref_t m_message;
	};

//
// make_transformed
//
/*!
 * \brief Helper function for creation of an instance of %transformed_message_t.
 *
 * It's just a simple way of calling transformed_message_t<Msg>::make().
 *
 * Usage example:
 * \code
 * // For a case of messages.
 * struct my_source_msg { ... };
 * struct my_result_msg { ... };
 *
 * so_5::bind_transformer< my_source_msg >(binding, src_mbox,
 * 	[dest_mbox](const auto & msg) {
 * 		// We can make a mutable message.
 * 		return so_5::make_transformed< so_5::mutable_msg< my_result_msg > >(
 * 				dest_mbox, // The first parameter is the destination mbox.
 * 				... // Remaining parameters will be forwarded to
 * 				    // the so_5::transformed_message_t< so_5::mutable_msg<my_result_msg> > constructor.
 * 			);
 * 	});
 *
 * // For a case of signals.
 * struct my_source_sig { ... };
 * struct my_result_sig { ... };
 *
 * so_5::bind_transformer< my_source_sig >(binding, src_mbox,
 * 	[dest_mbox]() { // NOTE: no args for transformer lambda!
 * 		return so_5::make_transformed< my_result_sig >(
 * 				dest_mbox // The first parameter is the destination mbox.
 * 				// No more parameters for a signal.
 * 			);
 * 	});
 * \endcode
 *
 * \tparam Msg type of the transformed (result) message.
 *
 * \tparam Args type of arguments to be passed to the
 * transformed_message_t<Msg>::make(). If \a Msg is a signal type
 * then \a Args has to be an empty type list.
 */
template< typename Msg, typename... Args >
[[nodiscard]]
transformed_message_t< Msg >
make_transformed(
	//! The destination mbox.
	mbox_t mbox,
	//! Parameters to be used for constructing an instance of \a Msg type.
	//! This can be an empty parameters list.
	Args &&... args )
	{
		return transformed_message_t< Msg >::make(
				std::move( mbox ),
				std::forward<Args>( args )... );
	}

/*!
 * \brief Helper function for creation of an instance of %transformed_message_t.
 *
 * It's just a simple way of calling transformed_message_t<Msg>::make().
 *
 * \note
 * This function has to be used when the result message is
 * represented as so_5::message_holder_t.
 *
 * Usage example:
 * \code
 * // For a case of messages.
 * struct my_source_msg { ... };
 * struct my_result_msg { ... };
 *
 * so_5::bind_transformer< my_source_msg >(binding, src_mbox,
 * 	[dest_mbox](const auto & msg) {
 * 		// We can make a mutable message.
 * 		auto result = so_5::message_holder_t< so_5::mutable_msg<my_result_msg> >::make(
 * 			... // Parameters to be used for my_result_msg construction.
 * 			);
 * 		return so_5::make_transformed(
 * 				dest_mbox, // The first parameter is the destination mbox.
 * 				std::move(result) // The transformed message.
 * 			);
 * 	});
 * \endcode
 *
 * \attention
 * This function can't be used for signals because signals are not
 * supported by so_5::message_holder_t.
 *
 * \tparam Msg type of the transformed (result) message.
 *
 * \tparam Ownership ownership of message instance in \a msg_holder (see
 * so_5::message_ownership_t for more details).
 */
template<
	typename Msg,
	message_ownership_t Ownership >
[[nodiscard]]
transformed_message_t< Msg >
make_transformed(
	//! The destination mbox.
	mbox_t mbox,
	//! Already constructed message.
	//! It is expected that \a msg_holder isn't an empty holder.
	message_holder_t< Msg, Ownership > msg_holder )
	{
		return transformed_message_t< Msg >{
				std::move( mbox ),
				std::move( msg_holder )
			};
	}

} /* namespace so_5 */

