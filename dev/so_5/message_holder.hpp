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

//FIXME: document this!
enum class message_ownership_t
	{
		autodetected,
		unique,
		shared
	};

namespace details
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

//FIXME: document this!
template< typename Payload, typename Envelope >
class shared_message_holder_impl_t
	{
		intrusive_ptr_t< Envelope > m_msg;

	protected :
		SO_5_NODISCARD
		const auto &
		message_reference() const noexcept { return m_msg; }

		SO_5_NODISCARD
		auto &
		message_reference() noexcept { return m_msg; }

	public :
		using payload_type = Payload;
		using envelope_type = Envelope;

		shared_message_holder_impl_t() noexcept = default;

		shared_message_holder_impl_t(
			intrusive_ptr_t< Envelope > msg )
			:	m_msg{ std::move(msg) }
			{}

		SO_5_NODISCARD
		intrusive_ptr_t< Envelope >
		make_reference() const noexcept
			{
				return m_msg;
			}

		void
		reset() noexcept
			{
				m_msg.reset();
			}
	};

//FIXME: document this!
template< typename Payload, typename Envelope >
class unique_message_holder_impl_t
	{
		intrusive_ptr_t< Envelope > m_msg;

	protected :
		SO_5_NODISCARD
		const auto &
		message_reference() const noexcept { return m_msg; }

		SO_5_NODISCARD
		auto &
		message_reference() noexcept { return m_msg; }

	public :
		using payload_type = Payload;
		using envelope_type = Envelope;

		unique_message_holder_impl_t() noexcept = default;

		unique_message_holder_impl_t(
			intrusive_ptr_t< Envelope > msg )
			:	m_msg{ std::move(msg) }
			{}

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

		SO_5_NODISCARD
		intrusive_ptr_t< Envelope >
		make_reference() noexcept
			{
				return { std::move(m_msg) }; 
			}

		void
		reset() noexcept
			{
				m_msg.reset();
			}
	};

//FIXME: document this!
template<
	typename Msg,
	message_ownership_t Ownership >
struct message_holder_impl_selector
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

//FIXME: document this!
template<
	typename Msg,
	message_ownership_t Ownership >
using message_holder_impl_selector_t =
		typename message_holder_impl_selector<Msg, Ownership>::type;

//FIXME: document this!
template< typename Base >
class const_only_accessors_t : public Base
	{
	public :
		using Base::Base;

		SO_5_NODISCARD
		const auto *
		get() const noexcept
			{
				return get_const_ptr( this->message_reference() );
			}

		SO_5_NODISCARD
		const auto &
		operator * () const noexcept { return *get(); }

		SO_5_NODISCARD
		const auto *
		operator->() const noexcept { return get(); }
	};

//FIXME: document this!
template< typename Base >
class non_const_accessors_t : public const_only_accessors_t<Base>
	{
		using direct_base_type = const_only_accessors_t<Base>;

	public:
		using direct_base_type::direct_base_type;

		using direct_base_type::get;
		using direct_base_type::operator*;
		using direct_base_type::operator->;

		SO_5_NODISCARD
		auto *
		get() noexcept
			{
				return get_non_const_ptr( this->message_reference() );
			}

		SO_5_NODISCARD
		auto &
		operator * () noexcept { return *get(); }

		SO_5_NODISCARD
		auto *
		operator->() noexcept { return get(); }
	};

//FIXME: document this!
template<
	message_mutability_t Mutability,
	typename Base >
struct message_holder_accessor_selector
	{
		using type = std::conditional_t<
				message_mutability_t::immutable_message == Mutability,
				const_only_accessors_t<Base>,
				non_const_accessors_t<Base> >;
	};

//FIXME: document this!
template<
	message_mutability_t Mutability,
	typename Base >
using message_holder_accessor_selector_t =
		typename message_holder_accessor_selector<Mutability, Base>::type;

} /* namespace details */

//FIXME: document this!
template<
	typename Msg,
	message_ownership_t Ownership = message_ownership_t::autodetected >
class message_holder_t
	:	public details::message_holder_accessor_selector_t<
				details::message_mutability_traits<Msg>::mutability,
				details::message_holder_impl_selector_t<Msg, Ownership> >
	{
		using base_type = details::message_holder_accessor_selector_t<
				details::message_mutability_traits<Msg>::mutability,
				details::message_holder_impl_selector_t<Msg, Ownership> >;

	public :
		using base_type::base_type;

		using payload_type = typename base_type::payload_type;
		using envelope_type = typename base_type::envelope_type;

		friend void
		swap( message_holder_t & a, message_holder_t & b ) noexcept
			{
				using std::swap;
				swap( a.message_reference(), b.message_reference() );
			}

		//! Create a new instance of message.
		template< typename... Args >
		SO_5_NODISCARD
		static message_holder_t
		make( Args && ...args )
			{
				using namespace details;

				intrusive_ptr_t< envelope_type > msg{
					make_message_instance< Msg >( std::forward<Args>(args)... )
				};
				mark_as_mutable_if_necessary< Msg >( *msg );

				return { std::move(msg) };
			}
	};

} /* namespace so_5 */

