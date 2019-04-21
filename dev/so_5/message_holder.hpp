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

/*!
 * \brief Type of ownership of a message instance inside message_holder.
 *
 * This type is intended to be used as parameter for message_holder_t
 * template.
 *
 * \since
 * v.5.6.0
 */
enum class message_ownership_t
	{
		//! Type of ownership will be automatically detected in dependency
		//! of message mutability.
		autodetected,
		//! An instance of message_holder should be the unique holder of
		//! the message instance.
		//! In that case message_holder will be similar to unique_ptr.
		unique,
		//! Several instances of message_holder can own the message instance.
		//! In that case message_holder will be similar to shared_ptr.
		shared
	};

namespace details
{

namespace message_holder_details
{

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
 * \brief Basic part of message_holder implementations.
 *
 * Contains method which should be present in all implementations.
 *
 * \since
 * v.5.6.0
 */
template< typename Payload, typename Envelope >
class basic_message_holder_impl_t
	{
		//! Message instance.
		/*!
		 * Can be empty if the message_holder doens't hold anything
		 * (in that case empty message_holder is an analogue of nullptr).
		 */
		intrusive_ptr_t< Envelope > m_msg;

	protected :
		//! Get reference to the underlying smart pointer to message instance.
		SO_5_NODISCARD
		const auto &
		message_reference() const noexcept { return m_msg; }

		//! Get reference to the underlying smart pointer to message instance.
		SO_5_NODISCARD
		auto &
		message_reference() noexcept { return m_msg; }

	public :
		using payload_type = Payload;
		using envelope_type = Envelope;

		basic_message_holder_impl_t() noexcept = default;

		basic_message_holder_impl_t(
			intrusive_ptr_t< Envelope > msg )
			:	m_msg{ std::move(msg) }
			{}

		//! Drops to pointer to the message instance.
		/*!
		 * The message_holder becomes empty as the result.
		 */
		void
		reset() noexcept
			{
				m_msg.reset();
			}

		//! Check for the emptiness of message_holder.
		bool
		empty() const noexcept
			{
				return static_cast<bool>( m_msg );
			}

		//! Check for the non-emptiness of message_holder.
		operator bool() const noexcept
			{
				return !this->empty();
			}

		//! Check for the emptiness of message_holder.
		bool operator!() const noexcept
			{
				return this->empty();
			}
	};

/*!
 * \brief A part of implementation of message_holder to be used for
 * shared ownership of message instances.
 *
 * This implementation allows copy and move.
 *
 * This implementation provides const make_reference() method that
 * returns a copy of underlying smart pointer.
 *
 * \since
 * v.5.6.0
 */
template< typename Payload, typename Envelope >
class shared_message_holder_impl_t
	:	public basic_message_holder_impl_t<Payload, Envelope>
	{
		using direct_base_type = basic_message_holder_impl_t<Payload, Envelope>;

	public :
		using direct_base_type::direct_base_type;

		//! Make an another reference to the message.
		/*!
		 * Returns empty smart pointer if message_holder is empty.
		 */
		SO_5_NODISCARD
		intrusive_ptr_t< Envelope >
		make_reference() const noexcept
			{
				return this->message_reference();
			}
	};

/*!
 * \brief A part of implementation of message_holder to be used for
 * unique ownership of message instances.
 *
 * This implementation allows only move operations and disables
 * copy constructor/operator.
 *
 * This implementation provides non-const make_reference() method that
 * extracts underlying smart pointer and leaves message_holder empty.
 *
 * \since
 * v.5.6.0
 */
template< typename Payload, typename Envelope >
class unique_message_holder_impl_t
	:	public basic_message_holder_impl_t<Payload, Envelope>
	{
		using direct_base_type = basic_message_holder_impl_t<Payload, Envelope>;

	public :
		using direct_base_type::direct_base_type;

		unique_message_holder_impl_t(
			const unique_message_holder_impl_t & ) = delete;

		unique_message_holder_impl_t(
			unique_message_holder_impl_t && ) = default;

		unique_message_holder_impl_t &
		operator=(
			const unique_message_holder_impl_t & ) = delete;

		unique_message_holder_impl_t &
		operator=(
			unique_message_holder_impl_t && ) = default;

		//! Extracts the smart pointer to the message.
		/*!
		 * Returns empty smart pointer if message_holder is empty.
		 *
		 * Leaves the message_holder instance empty.
		 */
		SO_5_NODISCARD
		intrusive_ptr_t< Envelope >
		make_reference() noexcept
			{
				return { std::move(this->message_reference()) }; 
			}
	};

/*!
 * \brief A meta-function for selection a base of message_holder implementation
 * in compile-time.
 *
 * If Ownership is message_ownership_t::autodetect then message's mutability
 * is examined. If message immutable then shared_message_holder_impl_t
 * will be selected as the base class. For mutable messages
 * unique_message_holder_impl_t will be used.
 *
 * \since
 * v.5.6.0
 */
template<
	typename Msg,
	message_ownership_t Ownership >
struct impl_selector
	{
		using P = typename message_payload_type< Msg >::payload_type;
		using E = typename message_payload_type< Msg >::envelope_type;

		using type = std::conditional_t<
				message_ownership_t::autodetected == Ownership,
					std::conditional_t<
							message_mutability_t::immutable_message ==
									message_mutability_traits<Msg>::mutability,
							shared_message_holder_impl_t<P, E>,
							unique_message_holder_impl_t<P, E> >,
					std::conditional_t<
							message_ownership_t::shared == Ownership,
							shared_message_holder_impl_t<P, E>,
							unique_message_holder_impl_t<P, E> >
			>;
	};

/*!
 * \brief Just a shortcut for impl_selector meta-function.
 *
 * \since
 * v.5.6.0
 */
template<
	typename Msg,
	message_ownership_t Ownership >
using impl_selector_t = typename impl_selector<Msg, Ownership>::type;

/*!
 * \brief An implementation of mixin for the case when only const-getters
 * are necessary for message_holder.
 *
 * It is assumed that shared_message_holder_impl_t or
 * unique_message_holder_impl_t will be used as Base template parameter.
 *
 * \since
 * v.5.6.0
 */
template< typename Base >
class const_only_accessors_t : public Base
	{
	public :
		using Base::Base;

		//! Get a pointer to the message inside message_holder.
		/*!
		 * \attention
		 * Returns nullptr is message_holder is empty.
		 */
		SO_5_NODISCARD
		const auto *
		get() const noexcept
			{
				return get_const_ptr( this->message_reference() );
			}

		//! Get a reference to the message inside message_holder.
		/*!
		 * \attention
		 * An attempt to use this method on empty message_holder is UB.
		 */
		SO_5_NODISCARD
		const auto &
		operator * () const noexcept { return *get(); }

		//! Get a pointer to the message inside message_holder.
		/*!
		 * \attention
		 * An attempt to use this method on empty message_holder is UB.
		 */
		SO_5_NODISCARD
		const auto *
		operator->() const noexcept { return get(); }
	};

/*!
 * \brief An implementation of mixin for the case when const-
 * and non-const getters are necessary for message_holder.
 *
 * It is assumed that shared_message_holder_impl_t or
 * unique_message_holder_impl_t will be used as Base template parameter.
 *
 * \since
 * v.5.6.0
 */
template< typename Base >
class non_const_accessors_t : public const_only_accessors_t<Base>
	{
		using direct_base_type = const_only_accessors_t<Base>;

	public:
		using direct_base_type::direct_base_type;

		using direct_base_type::get;
		using direct_base_type::operator*;
		using direct_base_type::operator->;

		//! Get a pointer to the message inside message_holder.
		/*!
		 * \attention
		 * Returns nullptr is message_holder is empty.
		 */
		SO_5_NODISCARD
		auto *
		get() noexcept
			{
				return get_non_const_ptr( this->message_reference() );
			}

		//! Get a reference to the message inside message_holder.
		/*!
		 * \attention
		 * An attempt to use this method on empty message_holder is UB.
		 */
		SO_5_NODISCARD
		auto &
		operator * () noexcept { return *get(); }

		//! Get a pointer to the message inside message_holder.
		/*!
		 * \attention
		 * An attempt to use this method on empty message_holder is UB.
		 */
		SO_5_NODISCARD
		auto *
		operator->() noexcept { return get(); }
	};

/*!
 * \brief A meta-function for selection of type of accessors mixin.
 *
 * The mutability of message is examined. const_only_accessors_t type
 * is selected if message is immutable. non_const_accessors_t type is
 * selected otherwise.
 *
 * \since
 * v.5.6.0
 */
template<
	message_mutability_t Mutability,
	typename Base >
struct accessor_selector
	{
		using type = std::conditional_t<
				message_mutability_t::immutable_message == Mutability,
				const_only_accessors_t<Base>,
				non_const_accessors_t<Base> >;
	};

/*!
 * \brief Just a shortcut for accessor_selector meta-function.
 *
 * \since
 * v.5.6.0
 */
template<
	message_mutability_t Mutability,
	typename Base >
using accessor_selector_t =
		typename accessor_selector<Mutability, Base>::type;

} /* namespace message_holder_details */

} /* namespace details */

//FIXME: document this!
template<
	typename Msg,
	message_ownership_t Ownership = message_ownership_t::autodetected >
class message_holder_t
	:	public details::message_holder_details::accessor_selector_t<
				details::message_mutability_traits<Msg>::mutability,
				details::message_holder_details::impl_selector_t<Msg, Ownership> >
	{
		using base_type = details::message_holder_details::accessor_selector_t<
				details::message_mutability_traits<Msg>::mutability,
				details::message_holder_details::impl_selector_t<Msg, Ownership> >;

	public :
		using payload_type = typename base_type::payload_type;
		using envelope_type = typename base_type::envelope_type;

		using base_type::base_type;

		//! Special constructor for constructing message_holder with
		//! a new message instance inside.
		template< typename... Args >
		message_holder_t(
			std::piecewise_construct_t,
			Args && ...args )
			:	base_type{ make_msg_instance( std::forward<Args>(args)... ) }
			{}

		friend void
		swap( message_holder_t & a, message_holder_t & b ) noexcept
			{
				using std::swap;
				swap( a.message_reference(), b.message_reference() );
			}

		//! Create a new instance of message_holder with a new message inside.
		template< typename... Args >
		SO_5_NODISCARD
		static message_holder_t
		make( Args && ...args )
			{
				return { make_msg_instance( std::forward<Args>(args)... ) };
			}

	private :
		//! Create a new instance of message.
		template< typename... Args >
		SO_5_NODISCARD
		static intrusive_ptr_t< envelope_type >
		make_msg_instance( Args && ...args )
			{
				using namespace details;

				intrusive_ptr_t< envelope_type > msg{
					make_message_instance< Msg >( std::forward<Args>(args)... )
				};
				mark_as_mutable_if_necessary< Msg >( *msg );

				return msg;
			}
	};

} /* namespace so_5 */

