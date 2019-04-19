/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Definition of the template class mhood.
*/

#pragma once

#include <so_5/message.hpp>

#include <so_5/compiler_features.hpp>

#include <type_traits>

namespace so_5
{

namespace details
{

template< typename Msg, message_mutability_t Mut >
class message_holder_impl_t;

//! A helper function to get a const raw pointer from smart pointer.
template< typename M >
const M *
get_const_ptr( const intrusive_ptr_t<M> & msg ) noexcept
	{
		return msg.get();
	}

//! A helper function to get a const raw pointer from smart pointer.
/*!
 * This overload is for case when M is a user type message.
 */
template< typename M >
const M *
get_const_ptr( const intrusive_ptr_t< user_type_message_t<M> > & msg ) noexcept
	{
		return std::addressof(msg->m_payload);
	}

//! A helper function to get a non-const raw pointer from smart pointer.
template< typename M >
M *
get_non_const_ptr( intrusive_ptr_t<M> & msg ) noexcept
	{
		return msg.get();
	}

//! A helper function to get a const raw pointer from smart pointer.
/*!
 * This overload is for case when M is a user type message.
 */
template< typename M >
M *
get_non_const_ptr( intrusive_ptr_t< user_type_message_t<M> > & msg ) noexcept
	{
		return std::addressof(msg->m_payload);
	}

/*!
 * \brief A specialization for immutable messages.
 *
 * \tparam Msg type of message.
 *
 * \since
 * v.5.5.14
 */
template< typename Msg >
class message_holder_impl_t<
		Msg,
		message_mutability_t::immutable_message > final
	{
	public :
		using payload_type = typename message_payload_type< Msg >::payload_type;
		using envelope_type = typename message_payload_type< Msg >::envelope_type;

		message_holder_impl_t() noexcept = default;

		message_holder_impl_t(
			intrusive_ptr_t< envelope_type > mf ) noexcept
			: m_msg{ std::move(mf) }
			{}

		//! Access to the message.
		const payload_type *
		get() const noexcept
			{
				return get_const_ptr( m_msg );
			}

		//! Create a smart pointer for the message envelope.
		intrusive_ptr_t< envelope_type >
		make_reference() const noexcept
			{
				return m_msg;
			}

		//! Access to the message.
		const payload_type &
		operator * () const noexcept { return *get(); }

		//! Access to the message via pointer.
		const payload_type *
		operator->() const noexcept { return get(); }

		//! Create a new instance of message.
		template< typename... Args >
		SO_5_NODISCARD
		static message_holder_impl_t
		make( Args && ...args )
			{
				intrusive_ptr_t< envelope_type > msg{
						make_message_instance< Msg >( std::forward<Args>(args)... )
				};
				// NOTE: there is no need to mark message as a mutable.

				return { std::move(msg) };
			}

	private :
		//! Actual message.
		intrusive_ptr_t< envelope_type > m_msg;
	};

/*!
 * \brief A specialization for mutable messages.
 *
 * \tparam Msg type of message.
 *
 * \since
 * v.5.5.14
 */
template< typename Msg >
class message_holder_impl_t<
		Msg,
		message_mutability_t::mutable_message > final
	{
	public :
		using payload_type = typename message_payload_type< Msg >::payload_type;
		using envelope_type = typename message_payload_type< Msg >::envelope_type;

		message_holder_impl_t() noexcept = default;

		message_holder_impl_t(
			intrusive_ptr_t< envelope_type > mf ) noexcept
			: m_msg{ std::move(mf) }
			{}

		message_holder_impl_t( const message_holder_impl_t & ) = delete;

		message_holder_impl_t( message_holder_impl_t && ) noexcept = default;

		message_holder_impl_t &
		operator=( const message_holder_impl_t & ) = delete;

		message_holder_impl_t &
		operator=( message_holder_impl_t && ) noexcept = default;

		friend void
		swap( message_holder_impl_t & a, message_holder_impl_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_msg, b.m_msg );
			}

		//! Access to the message.
		const payload_type *
		get() const noexcept
			{
				return get_const_ptr( m_msg );
			}

		//! Access to the message.
		payload_type *
		get() noexcept
			{
				return get_non_const_ptr( m_msg );
			}

		//! Create a smart pointer for the message envelope.
		/*!
		 * \note
		 * message_holder becomes empty!
		 */
		intrusive_ptr_t< envelope_type >
		make_reference() const noexcept
			{
				return { std::move(m_msg) };
			}

		//! Access to the message.
		const payload_type &
		operator * () const noexcept { return *get(); }

		payload_type &
		operator * () noexcept { return *get(); }

		//! Access to the message via pointer.
		const payload_type *
		operator->() const noexcept { return get(); }

		payload_type *
		operator->() noexcept { return get(); }

		//! Create a new instance of message.
		template< typename... Args >
		SO_5_NODISCARD
		static message_holder_impl_t
		make( Args && ...args )
			{
				intrusive_ptr_t< envelope_type > msg{
						make_message_instance< Msg >( std::forward<Args>(args)... )
				};
				// New message should be marked as mutable one.
				mark_as_mutable_if_necessary< Msg >( *msg );

				return { std::move(msg) };
			}

	private :
		//! Actual message.
		intrusive_ptr_t< envelope_type > m_msg;
	};

/*!
 * \brief A special detector of message immutability/mutability.
 *
 * \since
 * v.5.6.0
 */
template< typename T >
struct message_holder_mutability_detector
{
	static constexpr const message_mutability_t mutability =
		message_mutability_traits<T>::mutability;
};

} /* namespace details */

//FIXME: make this comment actual!
/*!
 * \brief A message wrapped to be used as type of argument for
 * event handlers.
 *
 * \tparam M type of message or signal.
 *
 * \note This class provides different sets of methods in dependency of
 * type \a M. If M is a type derived from so_5::message_t, then there
 * will be the following methods for immutable message M:
 * \code
	const M * get() const;
	const M * operator->() const;
	const M & operator*() const;
	so_5::intrusive_ptr_t< M > make_reference() const;
 * \endcode
 * If M is a type derived from so_5::signal_t, then there will no be methods
 * at all. It is because there is no actual message object for a signal.
 * If M is user-type message then there will be the following methods
 * for immutable message M:
 * \code
	const M * get() const;
	const M * operator->() const;
	const M & operator*() const;
	so_5::intrusive_ptr_t< so_5::user_type_message_t<M> > make_reference() const;
 * \endcode
 * For mutable message M there will be the following methods is M is
 * derived from so_5::message_t:
 * \code
	M * get();
	M * operator->();
	M & operator*();
	so_5::intrusive_ptr_t< M > make_reference();
 * \endcode
 * If mutable message M is not derived from so_5::message_t then there
 * will be the following methods:
 * \code
	M * get();
	M * operator->();
	M & operator*();
	so_5::intrusive_ptr_t< so_5::user_type_message_t<M> > make_reference();
 * \endcode
 *
 * \attention
 * Method make_reference for mhood_t<mutable_msg<M>> will leave mhood object
 * in nullptr state. It means that mhood must not be used after calling
 * make_reference().
 *
 * \note Class mhood_t can be used for redirection of messages of user types.
 * For example (since v.5.5.19):
	\code
	class redirector : public so_5::agent_t
	{
		...
		void some_event( mhood_t< std::string > evt ) {
			// Redirection of message to the different mbox.
			so_5::send(m_another_mbox, evt);
		}
	};
	\endcode
 * If mhood_t<mutable_msg<M>> is used then a redirection must be done this
 * way:
	\code
	class redirector : public so_5::agent_t
	{
		...
		void some_event( mhood_t< mutable_msg<std::string> > evt ) {
			// Redirection of message to the different mbox.
			so_5::send(m_another_mbox, std::move(evt));
			// NOTE: evt is nullptr at this point. It must not be used anymore.
		}
	};
	\endcode
 */
template< typename M >
using message_holder_t = details::message_holder_impl_t<
		M,
		details::message_mutability_traits<M>::mutability >;

} /* namespace so_5 */

