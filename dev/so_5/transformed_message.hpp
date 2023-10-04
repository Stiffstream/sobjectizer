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
//FIXME: Update the documentation for this class.
/*!
 * \brief A result of message transformation.
 *
 * \tparam Msg Type of result (transformed) message.
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

		//FIXME: document this!
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
//FIXME: document this!
template< typename Msg, typename... Args >
[[nodiscard]]
transformed_message_t< Msg >
make_transformed( mbox_t mbox, Args &&... args )
	{
		return transformed_message_t< Msg >::make(
				std::move( mbox ),
				std::forward<Args>( args )... );
	}

//FIXME: document this!
template<
	typename Msg,
	message_ownership_t Ownership >
[[nodiscard]]
transformed_message_t< Msg >
make_transformed(
	mbox_t mbox,
	message_holder_t< Msg, Ownership > msg_holder )
	{
		return transformed_message_t< Msg >{
				std::move( mbox ),
				std::move( msg_holder )
			};
	}

} /* namespace so_5 */
