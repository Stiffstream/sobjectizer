/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Definition of the template class message_holder.

	\since
	v.5.6.0
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
M *
get_ptr( const intrusive_ptr_t<M> & msg ) noexcept
	{
		return msg.get();
	}

//! A helper function to get a const raw pointer from smart pointer.
/*!
 * This overload is for case when M is a user type message.
 */
template< typename M >
M *
get_ptr( const intrusive_ptr_t< user_type_message_t<M> > & msg ) noexcept
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
		SO_5_NODISCARD
		bool
		empty() const noexcept
			{
				return static_cast<bool>( m_msg );
			}

		//! Check for the non-emptiness of message_holder.
		SO_5_NODISCARD
		operator bool() const noexcept
			{
				return !this->empty();
			}

		//! Check for the emptiness of message_holder.
		SO_5_NODISCARD
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
		static_assert( !is_signal<Msg>::value,
				"Signals can't be used with message_holder" );

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
 * \brief An implementation of mixin for the case when getters
 * for immutable messages are necessary for message_holder.
 *
 * It is assumed that shared_message_holder_impl_t or
 * unique_message_holder_impl_t will be used as Base template parameter.
 *
 * \since
 * v.5.6.0
 */
template< typename Base >
class immutable_msg_accessors_t : public Base
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
				return get_ptr( this->message_reference() );
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
 * \brief An implementation of mixin for the case getters for mutable
 * message are necessary for message_holder.
 *
 * It is assumed that shared_message_holder_impl_t or
 * unique_message_holder_impl_t will be used as Base template parameter.
 *
 * \since
 * v.5.6.0
 */
template< typename Base >
class mutable_msg_accessors_t : public Base
	{
	public :
		using Base::Base;

		//! Get a pointer to the message inside message_holder.
		/*!
		 * \attention
		 * Returns nullptr is message_holder is empty.
		 */
		SO_5_NODISCARD
		auto *
		get() const noexcept
			{
				return get_ptr( this->message_reference() );
			}

		//! Get a reference to the message inside message_holder.
		/*!
		 * \attention
		 * An attempt to use this method on empty message_holder is UB.
		 */
		SO_5_NODISCARD
		auto &
		operator * () const noexcept { return *get(); }

		//! Get a pointer to the message inside message_holder.
		/*!
		 * \attention
		 * An attempt to use this method on empty message_holder is UB.
		 */
		SO_5_NODISCARD
		auto *
		operator->() const noexcept { return get(); }
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
				immutable_msg_accessors_t<Base>,
				mutable_msg_accessors_t<Base> >;
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

/*!
 * \brief A class for holding an instance of a message.
 *
 * \attention
 * This class should be used with messages only. Signals are not supported
 * by that class.
 *
 * This class is intended for simplification of holding message instances
 * for some time and resending them later. For example:
 * \code
 * class my_actor final : public so_5::agent_t {
 * 	// A stored message.
 * 	so_5::message_holder_t<my_message> stored_;
 * 	...
 * 	void on_message(mhood_t<my_message> cmd) {
 * 		// Store message inside the agent.
 * 		stored_ = cmd.make_holder();
 * 		...
 * 		// Initiate a delayed message to resend the stored message later.
 * 		so_5::send_delayed<resend_message>(*this, 10s);
 * 	}
 * 	...
 * 	void on_resend_message(mhood_t<resend_message>) {
 * 		// Resend the stored message.
 * 		so_5::send(some_target, stored_);
 * 		// The stored message is no more needed.
 * 		stored_.reset();
 *
 * 		// Or we can write:
 * 		// so_5::send(some_target, std::move(stored_));
 * 	}
 * };
 * \endcode
 * This class is also intended to be used with preallocated messages:
 * \code
 * class prealloc_msg_demo final : public so_5::agent_t {
 * 	so_5::message_holder_t<request> request_;
 * 	...
 * 	prealloc_msg_demo(
 * 		context_t ctx,
 * 		... // Some other params.
 * 		) : request_{std::piecewise_construct, ...} // Preallocation of message.
 * 		{}
 *
 * 	void on_some_event(...) {
 * 		...
 * 		// It is time to send preallocated message.
 * 		so_5::send(some_target, request_);
 * 		...
 * 	}
 * };
 * \endcode
 *
 * The main benefit of that class is the ability to correctly handle
 * messages of arbitrary user types (e.g. messages not derived from
 * so_5::message_t class) and mutability flags. For example, the following
 * cases are correctly handled by message_holder_t:
 * \code
 * struct status_data { // This is message that is not derived from so_5::message_t.
 * 	... // Some fields.
 * };
 *
 * so_5::message_holder_t<status_data> msg1;
 * so_5::message_holder_t<so_5::immutable_msg<status_data>> msg2;
 * so_5::message_holder_t<so_5::mutable_msg<status_data>> msg3;
 * \endcode
 *
 * \par Methods of message_holder_t class
 *
 * Class message_holder_t provides the following methods:
 * \code
 * // Default constructor. Creates an empty holder.
 * message_holder_t();
 *
 * // Constructs holder for holding the specified message instance.
 * message_holder_t(so_5::intrusive_ptr<envelope_type> msg);
 *
 * // Creates a new instance of message from 'args' and constructs holder for it.
 * template<typename... Args>
 * message_holder_t(std::piecewise_construct_t, Args && ...args);
 *
 * // Creates a new instance of message from 'args' and constructs holder for it.
 * template<typename... Args>
 * static message_holder_t make(Args && ...args);
 *
 * // Returns true if message_holder is empty.
 * bool empty() const noexcept;
 * bool operator!() const noexcept;
 *
 * // Returns true if message_holder is not empty.
 * operator bool() const noexcept;
 *
 * // Drops the content of message_holder.
 * void reset() noexcept;
 * \endcode
 *
 * There are also some more methods which are depend on mutability of
 * message and the type of ownership. They are described below.
 *
 * \par Getters are depend on mutability of message
 *
 * If a message_holder holds an immutable message then there are the following
 * getter methods:
 * \code
 * const payload_type * get() const noexcept;
 * const payload_type & operator*() const noexcept;
 * const payload_type * operator->() const noexcept;
 * \endcode
 * But if message_holder holds a mutable message those getters are still here
 * but they have non-const return type:
 * \code
 * payload_type * get() const noexcept;
 * payload_type & operator*() const noexcept;
 * payload_type * operator->() const noexcept;
 * \endcode
 *
 * \par Shared and unique ownership
 *
 * A message_holder works like a smart pointer. But what kind of smart pointer?
 *
 * It depends on Ownership template parameters.
 *
 * But default Ownership is so_5::message_ownership_t::autodetected.
 * In this case the behavior of a message_holder depends of the mutability
 * of message. If message is immutable then message_holders is like
 * std::shared_ptr: several message_holders can hold pointers to the
 * same message instances.
 *
 * If message is mutable then message_holder is like
 * std::unique_ptr: only one message_holder can hold a pointer to a message
 * instance.
 *
 * For example:
 * \code
 * // Immutable message.
 * so_5::message_holder_t<my_msg> msg1{std::piecewise_construct, ...};
 * so_5::message_holder_t<my_msg> msg2{ msg1 };
 * assert(msg1.get() == msg2.get()); // Now msg1 and msg2 refer to the same msg.
 *
 * // Mutable message.
 * so_5::message_holder_t<so_5::mutable_msg<my_msg>> msg3{...};
 * so_5::message_holder_t<so_5::mutable_msg<my_msg>> msg4{ msg3 }; // WON'T COMPILE!
 * so_5::message_holder_t<so_5::mutable_msg<my_msg>> msg5{ std::move(msg3) };
 * assert(msg3.empty()); // Now msg3 is empty.
 * assert(!msg5.empty()); // And only msg5 holds the message.
 * \endcode
 *
 * The value of Ownership parameter can be specified manually.
 * In that case we can have an unique-holder for an immutable message:
 * \code
 * so_5::message_holder_t<my_msg, so_5::message_ownership_t::unique> msg1{...};
 * // WON'T COMPILE!
 * so_5::message_holder_t<my_msg, so_5::message_ownership_t::unique> msg2{ msg1 };
 * // Will compile but ownership will be moved: 
 * so_5::message_holder_t<my_msg, so_5::message_ownership_t::unique> msg3{ std::move(msg) };
 * \endcode
 * There can also be a shared-holder for a mutable message:
 * \code
 * so_5::message_holder_t<my_msg, so_5::message_ownership_t::shared> msg1{...};
 * // No problems.
 * so_5::message_holder_t<my_msg, so_5::message_ownership_t::shared> msg2{ msg1 };
 * \endcode
 * But this approach should be taken with an additional care because it allows
 * to make several sends of the same mutable message instances at the same time.
 *
 * If a message_holder works as std::shared_ptr then there is the following
 * methods:
 * \code
 * // Copy constructor and operator.
 * message_holder_t(const message_holder_t &) noexcept;
 * message_holder_t & operator=(const message_holder_t &) noexcept;
 * // Move constructor and operator.
 * message_holder_t(message_holder_t &&) noexcept;
 * message_holder_t & operator=(message_holder_t &&) noexcept;
 *
 * // Getter for the underlying smart pointer.
 * intrusive_ptr_t<envelope_type> make_reference() const noexcept;
 * \endcode
 *
 * If a message_holder works as std::unique_ptr then copy operator/constructors
 * are disabled and make_reference() leaves the message_holder object empty:
 * \code
 * // Move constructor and operator.
 * message_holder_t(message_holder_t &&) noexcept;
 * message_holder_t & operator=(message_holder_t &&) noexcept;
 *
 * // Extracts the underlying smart pointer.
 * // Leaves the message_holder object empty.
 * intrusive_ptr_t<envelope_type> make_reference() noexcept;
 * \endcode
 *
 * \par Creation of an instance of message to be stored inside a message_holder
 *
 * There are several ways of creation of a message to be stored inside
 * a message_holder object.
 *
 * The recommended way is to use the constructor of message_holder with
 * std::piecewise_construct_t argument. This constructor automatically
 * creates an underlying message instance:
 * \code
 * struct my_msg {
 * 	int a_;
 * 	std::string b_;
 * };
 * so_5::message_holder_t<my_msg> msg{std::piecewise_construct,
 * 	0, // value for my_msg's a_ field.
 * 	"hello" // value for my_msg's b_ field.
 * };
 * \endcode
 * Sometimes a static method make() can be used for similar purpose:
 * \code
 * auto make_message() {
 * 	return so_5::message_holder_t<my_msg>::make(0, "hello");
 * }
 * \endcode
 *
 * But sometimes an instance of message is present as raw pointer,
 * std::unique_ptr or so_5::intrusive_ptr_t objects. In that case the
 * constructor that accepts intrusive_ptr_t can be used:
 * \code
 * // Somewhere in 3rd-party library.
 * std::unique_ptr<some_message> make_message() {
 * 	return std::make_unique<some_message>(...);
 * }
 *
 * // Somewhere in your code.
 * so_5::message_holder_t<some_message> msg{make_message()};
 * \endcode 
 *
 * \since
 * v.5.6.0
 */
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
		/*!
		 * Usage example:
		 * \code
		 * struct my_message {
		 * 	int a_;
		 * 	std::string b_;
		 * 	std::chrono::millisecons c_;
		 * };
		 *
		 * so_5::message_holder_t<my_message> msg{ std::piecewise_construct,
		 * 		0, // value for my_message's a_ field.
		 * 		"hello", // value for my_message's b_ field.
		 * 		15s // value for my_message's c_ field.
		 * };
		 * \endcode
		 */
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
		/*!
		 * Usage example:
		 * \code
		 * struct my_message {
		 * 	int a_;
		 * 	std::string b_;
		 * 	std::chrono::millisecons c_;
		 * };
		 *
		 * auto make_message() {
		 * 	return so_5::message_holder_t<my_message>(
		 * 		0, // value for my_message's a_ field.
		 * 		"hello", // value for my_message's b_ field.
		 * 		15s ); // value for my_message's c_ field.
		 * }
		 * \endcode
		 */
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

