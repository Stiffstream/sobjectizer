/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Public part of message chain related stuff.
 *
 * \since v.5.5.13
 */

#pragma once

#include <so_5/mbox.hpp>
#include <so_5/handler_makers.hpp>
#include <so_5/exception_control_flags.hpp>

#include <so_5/fwd.hpp>

#include <so_5/details/invoke_noexcept_code.hpp>
#include <so_5/details/remaining_time_counter.hpp>

#include <chrono>
#include <functional>

namespace so_5 {

namespace mchain_props {

/*!
 * \brief An alias for type for repesenting timeout values.
 *
 * \since v.5.5.13
 */
using duration_t = std::chrono::high_resolution_clock::duration;

namespace details {

//
// no_wait_special_timevalue
//
/*!
 * \brief Special value of %duration to indicate 'no_wait' case.
 *
 * \since v.5.5.13
 */
inline duration_t
no_wait_special_timevalue() { return duration_t::zero(); }

//
// infinite_wait_special_timevalue
//
/*!
 * \brief Special value of %duration to indicate 'infinite_wait' case.
 *
 * \since v.5.5.13
 */
inline duration_t
infinite_wait_special_timevalue() { return duration_t::max(); }

//
// is_no_wait_timevalue
//
/*!
 * \brief Is time value means 'no_wait'?
 *
 * \since v.5.5.13
 */
inline bool
is_no_wait_timevalue( duration_t v )
	{
		return v == no_wait_special_timevalue();
	}

//
// is_infinite_wait_timevalue
//
/*!
 * \brief Is time value means 'infinite_wait'?
 *
 * \since v.5.5.13
 */
inline bool
is_infinite_wait_timevalue( duration_t v )
	{
		return v == infinite_wait_special_timevalue();
	}

//
// actual_timeout
//

/*!
 * \brief Helper function for detection of actual value for waiting timeout.
 *
 * \note This helper implements convention that infinite waiting is
 * represented as duration_t::max() value.
 *
 * \since v.5.5.13
 */
inline duration_t
actual_timeout( infinite_wait_indication )
	{
		return infinite_wait_special_timevalue();
	}

/*!
 * \brief Helper function for detection of actual value for waiting timeout.
 *
 * \note This helper implements convention that no waiting is
 * represented as duration_t::zero() value.
 *
 * \since v.5.5.13
 */
inline duration_t
actual_timeout( no_wait_indication )
	{
		return no_wait_special_timevalue();
	}

/*!
 * \brief Helper function for detection of actual value for waiting timeout.
 *
 * \since v.5.5.13
 */
template< typename V >
duration_t
actual_timeout( V value )
	{
		return duration_t( value );
	}

} /* namespace details */

//
// demand_t
//
/*!
 * \brief Description of one demand in message chain.
 *
 * \since v.5.5.13
 */
struct demand_t
	{
		//! Type of the message.
		std::type_index m_msg_type;
		//! Event incident.
		so_5::message_ref_t m_message_ref;

		//! Default constructor.
		demand_t()
			:	m_msg_type( typeid(void) )
			{}
		//! Initializing constructor.
		demand_t(
			std::type_index msg_type,
			so_5::message_ref_t message_ref )
			:	m_msg_type{ std::move(msg_type) }
			,	m_message_ref{ std::move(message_ref) }
			{}

		//! Swap operation.
		friend void
		swap( demand_t & a, demand_t & b ) noexcept
			{
				using std::swap;

				swap( a.m_msg_type, b.m_msg_type );
				swap( a.m_message_ref, b.m_message_ref );
			}
	};

//
// memory_usage_t
//
/*!
 * \brief Memory allocation for storage for size-limited chains.
 *
 * \since v.5.5.13
 */
enum class memory_usage_t
	{
		//! Storage can be allocated and deallocated dynamically.
		dynamic,
		//! Storage must be preallocated once and doesn't change after that.
		preallocated
	};

//
// overflow_reaction_t
//
/*!
 * \brief What reaction must be performed on attempt to push new message to
 * the full message chain.
 *
 * \since v.5.5.13
 */
enum class overflow_reaction_t
	{
		//! Application must be aborted.
		abort_app,
		//! An exception must be thrown.
		/*!
		 * \note
		 * Since v.5.5.18 this value leads to an exception only if
		 * ordinary `send` is used for pushing message to overloaded
		 * message chain. If there is an attempt to push
		 * delayed or periodic message to overloaded message chain then
		 * throw_exception reaction is replaced by drop_newest. It is becasue
		 * the context of timer thread is a special contex. No exceptions
		 * should be thrown on it.
		 */
		throw_exception,
		//! New message must be ignored and droped.
		drop_newest,
		//! Oldest message in chain must be removed.
		remove_oldest
	};

//
// capacity_t
//
/*!
 * \brief Parameters for defining chain size.
 *
 * \since v.5.5.13
 */
class capacity_t
	{
		//! Has chain unlimited size?
		bool m_unlimited = { true };

		// NOTE: all other atributes have sence only if m_unlimited != true.

		//! Max size of the chain with limited size.
		std::size_t m_max_size;

		//! Type of the storage for size-limited chain.
		memory_usage_t m_memory;

		//! Type of reaction for chain overflow.
		overflow_reaction_t m_overflow_reaction;

		//! Timeout for waiting on full chain during 'message push' operation.
		/*!
		 * \note Value 'zero' means that there must not be waiting on
		 * full chain.
		 */
		duration_t m_overflow_timeout;

		//! Initializing constructor for size-limited message chain.
		capacity_t(
			std::size_t max_size,
			memory_usage_t memory_usage,
			overflow_reaction_t overflow_reaction,
			duration_t overflow_timeout )
			:	m_unlimited{ false }
			,	m_max_size{ max_size }
			,	m_memory{ memory_usage }
			,	m_overflow_reaction{ overflow_reaction }
			,	m_overflow_timeout( overflow_timeout )
			{}

	public :
		//! Default constructor.
		/*!
		 * Creates description for size-unlimited chain.
		 */
		capacity_t()
			{}

		//! Create capacity description for size-unlimited message chain.
		inline static capacity_t
		make_unlimited() { return capacity_t{}; }

		//! Create capacity description for size-limited message chain
		//! without waiting on full queue during 'push message' operation.
		inline static capacity_t
		make_limited_without_waiting(
			//! Max size of the chain.
			std::size_t max_size,
			//! Type of chain storage.
			memory_usage_t memory_usage,
			//! Reaction on chain overflow.
			overflow_reaction_t overflow_reaction )
			{
				return capacity_t{
						max_size,
						memory_usage,
						overflow_reaction,
						details::no_wait_special_timevalue()
				};
			}

		//! Create capacity description for size-limited message chain
		//! with waiting on full queue during 'push message' operation.
		inline static capacity_t
		make_limited_with_waiting(
			//! Max size of the chain.
			std::size_t max_size,
			//! Type of chain storage.
			memory_usage_t memory_usage,
			//! Reaction on chain overflow.
			overflow_reaction_t overflow_reaction,
			//! Waiting time on full message chain.
			duration_t wait_timeout )
			{
				return capacity_t{
						max_size,
						memory_usage,
						overflow_reaction,
						wait_timeout
				};
			}

		//! Is message chain have no size limit?
		bool
		unlimited() const { return m_unlimited; }

		//! Max size for size-limited chain.
		/*!
		 * \attention Has sence only for size-limited chain.
		 */
		std::size_t
		max_size() const { return m_max_size; }

		//! Memory allocation type for size-limited chain.
		/*!
		 * \attention Has sence only for size-limited chain.
		 */
		memory_usage_t
		memory_usage() const { return m_memory; }

		//! Overflow reaction for size-limited chain.
		/*!
		 * \attention Has sence only for size-limited chain.
		 */
		overflow_reaction_t
		overflow_reaction() const { return m_overflow_reaction; }

		//! Is waiting timeout for overflow case defined?
		/*!
		 * \attention Has sence only for size-limited chain.
		 */
		bool
		is_overflow_timeout_defined() const
			{
				return !details::is_no_wait_timevalue( m_overflow_timeout );
			}

		//! Get the value of waiting timeout for overflow case.
		/*!
		 * \attention Has sence only for size-limited chain.
		 */
		duration_t
		overflow_timeout() const
			{
				return m_overflow_timeout;
			}
	};

//
// extraction_status_t
//
/*!
 * \brief Result of extraction of message from a message chain.
 *
 * \since v.5.5.13
 */
enum class extraction_status_t
	{
		//! No available messages in the chain.
		no_messages,
		//! Message extracted successfully.
		msg_extracted,
		//! Message cannot be extracted because chain is closed.
		chain_closed
	};

//
// push_status_t
//
/*!
 * \brief Result of attempt of pushing a message into a message chain.
 *
 * \since v.5.7.0
 */
enum class push_status_t
	{
		//! Message wasn't stored.
		not_stored,
		//! Message stored into a message chain.
		stored,
		//! Message is not stored but the store operation is registered
		//! into a message chain.
		deffered,
		//! Message wasn't stored because chain is closed.
		chain_closed
	};

//
// close_mode_t
//
/*!
 * \brief What to do with chain's content at close.
 *
 * \since v.5.5.13
 */
enum class close_mode_t
	{
		//! All messages must be removed from chain.
		drop_content,
		//! All messages must be retained until they will be
		//! processed at receiver's side.
		retain_content
	};

//
// not_empty_notification_func_t
//
/*!
 * \brief Type of functor for notifies about arrival of a message to
 * the empty chain.
 *
 * \attention This function must be noexcept.
 *
 * \since v.5.5.13
 */
using not_empty_notification_func_t = std::function< void() >;

//
// Forward declarations related to multi chain select operations.
//
class select_case_t;

} /* namespace mchain_props */

//
// abstract_message_chain_t
//
/*!
 * \brief An interace of message chain.
 *
 * \since v.5.5.13
 */
class SO_5_TYPE abstract_message_chain_t : protected so_5::abstract_message_box_t
	{
		friend class intrusive_ptr_t< abstract_message_chain_t >;
		friend class mchain_props::select_case_t;

		abstract_message_chain_t( const abstract_message_chain_t & ) = delete;
		abstract_message_chain_t &
		operator=( const abstract_message_chain_t & ) = delete;

	protected :
		abstract_message_chain_t() = default;
		~abstract_message_chain_t() noexcept override = default;

	public :
		using abstract_message_box_t::id;
		using abstract_message_box_t::environment;

		[[nodiscard]]
		virtual mchain_props::extraction_status_t
		extract(
			//! Destination for extracted messages.
			mchain_props::demand_t & dest,
			//! Max time to wait on empty queue.
			mchain_props::duration_t empty_queue_timeout ) = 0;

		//! Cast message chain to message box.
		[[nodiscard]]
		so_5::mbox_t
		as_mbox();

		//! Is message chain empty?
		[[nodiscard]]
		virtual bool
		empty() const = 0;

		//! Count of messages in the chain.
		[[nodiscard]]
		virtual std::size_t
		size() const = 0;

		//! Close the chain.
		/*!
		 * Since v.5.7.3 this is the recommended way of closing a mchain.
		 * This method allows to specify how exceptions have to be treated:
		 * \code
		 * auto ch1 = so_5::create_mchain(env);
		 * auto ch2 = so_5::create_mchain(env);
		 * ...
		 * // Enable exceptions during 'close' operation.
		 * ch1->close(
		 * 		so_5::exceptions_enabled,
		 * 		so_5::mchain_props::close_mode_t::drop_content );
		 *
		 * // Terminate the application if 'close' operation throws.
		 * ch2->close(
		 * 		so_5::terminate_if_throws,
		 * 		so_5::mchain_props::close_mode_t::drop_content );
		 * \endcode
		 *
		 * This method was introduced because 'close' operation can throw but
		 * it's not obvious enough. Because 'close' can throw additional care has
		 * to be taken when 'close' is initiated in noexcept-contexts (like
		 * destructor and, often, in catch-blocks). This new method allows
		 * expressing intentions more clearly. For example:
		 * \code
		 * class some_class {
		 * 	so_5::mchain_t ch_;
		 * 	...
		 * public:
		 * 	some_class(so_5::environment_t & env)
		 * 		: ch_{so_5::create_mchain(env)}
		 * 	{}
		 * 	~some_class() noexcept
		 * 	{
		 * 		// There is no sense to allow exceptions during close operation.
		 * 		ch_->close(
		 * 				so_5::terminate_if_throws,
		 * 				so_5::mchain_props::close_mode_t::drop_content );
		 * 	}
		 * 	...
		 * };
		 * ...
		 * void some_function(so_5::environment_t & env)
		 * {
		 * 	auto ch = so_5::create_mchain(env);
		 * 	...
		 * 	// Have to close ch.
		 * 	// This is not noexcept-context, so we can allow exceptions.
		 * 	ch_->close(
		 * 			so_5::exceptions_enabled,
		 * 			so_5::mchain_props::close_mode_t::retain_content );
		 * 	...
		 * }
		 * \endcode
		 *
		 * \since v.5.7.3
		 */
		template< typename Exceptions_Control >
		void
		close(
			//! Are exceptions enabled or should the application be
			//! terminated if 'close' throws.
			Exceptions_Control /*exceptions_control*/,
			//! What to do with chain's content.
			mchain_props::close_mode_t mode )
			noexcept(noexcept(
					details::should_terminate_if_throws_t<Exceptions_Control>::value))
		{
			//NOTE: call close() from previous versions until v.5.8.0
			//will be released.
			this->actual_close( mode );
		}

	protected :
		/*!
		 * \brief An extraction attempt as a part of multi chain select.
		 *
		 * \attention
		 * This method is a pure virtual since v.5.6.2.
		 *
		 * \note This method is intended to be used by select_case_t.
		 *
		 * \since v.5.5.16
		 */
		[[nodiscard]]
		virtual mchain_props::extraction_status_t
		extract(
			//! Destination for extracted messages.
			mchain_props::demand_t & dest,
			//! Select case to be stored for notification if mchain is empty.
			mchain_props::select_case_t & select_case ) = 0;

		/*!
		 * \brief An attempt to push a new message into the mchain.
		 *
		 * Unlike do_deliver_message() method the push() doesn't apply
		 * the overload reaction if the mchain if full. The \a select_case
		 * is stored to select_cases list instead.
		 *
		 * \note
		 * This method is intended to be used by select_case_t.
		 *
		 * \since v.5.7.0
		 */
		[[nodiscard]]
		virtual mchain_props::push_status_t
		push(
			//! Type of message/signal to be pushed.
			const std::type_index & msg_type,
			//! Message/signal to be pushed.
			const message_ref_t & message,
			//! Select case to be stored for notification if mchain is full.
			mchain_props::select_case_t & select_case ) = 0;

		/*!
		 * \brief Removement of mchain from multi chain select.
		 *
		 * \attention
		 * This method is a pure virtual and noexcept since v.5.6.2.
		 *
		 * \note This method is intended to be used by select_case_t.
		 *
		 * \since v.5.5.16
		 */
		virtual void
		remove_from_select(
			//! Select case to be removed from notification queue.
			mchain_props::select_case_t & select_case ) noexcept = 0;

		//! Close the chain.
		/*!
		 * \attention
		 * This method can throw exception.
		 *
		 * \since v.5.7.3
		 */
		virtual void
		actual_close(
			//! What to do with chain's content.
			mchain_props::close_mode_t mode ) = 0;
	};

//
// mchain_t
//
/*!
 * \brief Short name for smart pointer to message chain.
 *
 * \since v.5.5.13
 */
using mchain_t = intrusive_ptr_t< abstract_message_chain_t >;

//
// close_drop_content
//
/*!
 * \brief Helper function for closing a message chain with dropping
 * all its content.
 *
 * Usage example.
	\code
	so_5::mchain_t & ch = ...;
	... // Some work with chain.
	close_drop_content( so_5::exceptions_enabled, ch );
	// Or:
	ch->close(
		so_5::exceptions_enabled,
		so_5::mchain_props::close_mode_t::drop_content );
	\endcode
 *
 * \note Because of ADL it can be used without specifying namespaces.
 *
 * \tparam Exceptions_Control it is expected to be so_5::exceptions_enabled_t or
 * so_5::terminate_if_throws_t. So the first parameter to close_drop_content
 * has to be so_5::exceptions_enabled or so_5::terminate_if_throws.
 *
 * \since v.5.7.3
 */
template< typename Exceptions_Control >
inline void
close_drop_content(
	//! What to do with exceptions.
	//! It's expected to be so_5::exceptions_enabled or so_5::terminate_if_throws.
	Exceptions_Control exceptions_control,
	//! Chain to be closed.
	const mchain_t & ch )
	noexcept(noexcept(
			details::should_terminate_if_throws_t<Exceptions_Control>::value))
	{
		ch->close(
				exceptions_control,
				mchain_props::close_mode_t::drop_content );
	}

//
// close_retain_content
//
/*!
 * \brief Helper function for closing a message chain with retaining
 * all its content.
 *
 * \note Because of ADL it can be used without specifying namespaces.
 *
 * \par Usage example.
	\code
	so_5::mchain_t & ch = ...;
	... // Some work with chain.
	close_retain_content( so_5::exceptions_enabled, ch );
	// Or:
	ch->close(
		so_5::exceptions_enabled,
		so_5::mchain_props::close_mode_t::retain_content );
	\endcode
 *
 * \tparam Exceptions_Control it is expected to be so_5::exceptions_enabled_t or
 * so_5::terminate_if_throws_t. So the first parameter to close_drop_content
 * has to be so_5::exceptions_enabled or so_5::terminate_if_throws.
 *
 * \since v.5.7.3
 */
template< typename Exceptions_Control >
inline void
close_retain_content(
	//! What to do with exceptions.
	//! It's expected to be so_5::exceptions_enabled or so_5::terminate_if_throws.
	Exceptions_Control exceptions_control,
	//! Chain to be closed.
	const mchain_t & ch )
	noexcept(noexcept(
			details::should_terminate_if_throws_t<Exceptions_Control>::value))
	{
		ch->close( exceptions_control, mchain_props::close_mode_t::retain_content );
	}

//
// mchain_params_t
//
/*!
 * \since v.5.5.13
 *
 * \brief Parameters for message chain.
 */
class mchain_params_t
	{
		//! Chain's capacity.
		mchain_props::capacity_t m_capacity;

		//! An optional notificator for 'not_empty' condition.
		mchain_props::not_empty_notification_func_t m_not_empty_notificator;

		//! Is message delivery tracing disabled explicitly?
		bool m_msg_tracing_disabled = { false };

	public :
		//! Initializing constructor.
		mchain_params_t(
			//! Chain's capacity and related params.
			mchain_props::capacity_t capacity )
			:	m_capacity{ capacity }
			{}

		//! Set chain's capacity and related params.
		mchain_params_t &
		capacity( mchain_props::capacity_t capacity )
			{
				m_capacity = capacity;
				return *this;
			}

		//! Get chain's capacity and related params.
		const mchain_props::capacity_t &
		capacity() const
			{
				return m_capacity;
			}

		//! Set chain's notificator for 'not_empty' condition.
		/*!
		 * This notificator will be called when a message is stored to
		 * the empty chain and chain becomes not empty.
		 */
		mchain_params_t &
		not_empty_notificator(
			mchain_props::not_empty_notification_func_t notificator )
			{
				m_not_empty_notificator = std::move(notificator);
				return *this;
			}

		//! Get chain's notificator for 'not_empty' condition.
		const mchain_props::not_empty_notification_func_t &
		not_empty_notificator() const
			{
				return m_not_empty_notificator;
			}

		//! Disable message delivery tracing explicitly.
		/*!
		 * If this method called then message delivery tracing will
		 * not be used for that mchain even if message delivery
		 * tracing will be used for the whole SObjectizer Environment.
		 */
		mchain_params_t &
		disable_msg_tracing()
			{
				m_msg_tracing_disabled = true;
				return *this;
			}

		//! Is message delivery tracing disabled explicitly?
		bool
		msg_tracing_disabled() const
			{
				return m_msg_tracing_disabled;
			}
	};

/*!
 * \name Helper functions for creating parameters for %mchain.
 * \{
 */

/*!
 * \brief Create parameters for size-unlimited %mchain.
 *
 * \par Usage example:
	\code
	so_5::environment_t & env = ...;
	auto chain = env.create_mchain( so_5::make_unlimited_mchain_params() );
	\endcode
 *
 * \since v.5.5.13
 */
inline mchain_params_t
make_unlimited_mchain_params()
	{
		return mchain_params_t{ mchain_props::capacity_t::make_unlimited() };
	}

/*!
 * \brief Create parameters for size-limited %mchain without waiting on overflow.
 *
 * \par Usage example:
	\code
	so_5::environment_t & env = ...;
	auto chain = env.create_mchain( so_5::make_limited_without_waiting_mchain_params(
			// No more than 200 messages in the chain.
			200,
			// Memory will be allocated dynamically.
			so_5::mchain_props::memory_usage_t::dynamic,
			// New messages will be ignored on chain's overflow.
			so_5::mchain_props::overflow_reaction_t::drop_newest ) );
	\endcode
 *
 * \since v.5.5.13
 */
inline mchain_params_t
make_limited_without_waiting_mchain_params(
	//! Max capacity of %mchain.
	std::size_t max_size,
	//! Type of chain storage.
	mchain_props::memory_usage_t memory_usage,
	//! Reaction on chain overflow.
	mchain_props::overflow_reaction_t overflow_reaction )
	{
		return mchain_params_t{
				mchain_props::capacity_t::make_limited_without_waiting(
						max_size,
						memory_usage,
						overflow_reaction )
		};
	}

/*!
 * \brief Create parameters for size-limited %mchain with waiting on overflow.
 *
 * \par Usage example:
	\code
	so_5::environment_t & env = ...;
	auto chain = env.create_mchain( so_5::make_limited_with_waiting_mchain_params(
			// No more than 200 messages in the chain.
			200,
			// Memory will be preallocated.
			so_5::mchain_props::memory_usage_t::preallocated,
			// New messages will be ignored on chain's overflow.
			so_5::mchain_props::overflow_reaction_t::drop_newest,
			// But before dropping a new message there will be 500ms timeout
			std::chrono::milliseconds(500) ) );
	\endcode
 *
 * \note
 * Since v.5.5.18 there is an important difference in mchain behavior.
 * If an ordinary `send` is used for message pushing then there will
 * be waiting for free space if the message chain is full.
 * But if message push is performed from timer thread (it means that
 * message is a delayed or a periodic message) then there will not be
 * any waiting. It is because the context of timer thread is very
 * special: there is no possibility to spend some time on waiting for
 * some free space in message chain. All operations on the context of
 * timer thread must be done as fast as possible.
 *
 * \since v.5.5.13
 */
inline mchain_params_t
make_limited_with_waiting_mchain_params(
	//! Max size of the chain.
	std::size_t max_size,
	//! Type of chain storage.
	mchain_props::memory_usage_t memory_usage,
	//! Reaction on chain overflow.
	mchain_props::overflow_reaction_t overflow_reaction,
	//! Waiting time on full message chain.
	mchain_props::duration_t wait_timeout )
	{
		return mchain_params_t {
				mchain_props::capacity_t::make_limited_with_waiting(
						max_size,
						memory_usage,
						overflow_reaction,
						wait_timeout )
		};
	}

/*!
 * \}
 */

//
// mchain_receive_result_t
//
/*!
 * \brief A result of receive from %mchain.
 *
 * \since v.5.5.13
 */
class mchain_receive_result_t
	{
		//! Count of extracted messages.
		std::size_t m_extracted;
		//! Count of handled messages.
		std::size_t m_handled;
		//! Extraction status (e.g. no messages, chain closed and so on).
		mchain_props::extraction_status_t m_status;

	public :
		//! Default constructor.
		mchain_receive_result_t() noexcept
			:	m_extracted{ 0 }
			,	m_handled{ 0 }
			,	m_status{ mchain_props::extraction_status_t::no_messages }
			{}

		//! Initializing constructor.
		mchain_receive_result_t(
			//! Count of extracted messages.
			std::size_t extracted,
			//! Count of handled messages.
			std::size_t handled,
			//! Status of extraction operation.
			mchain_props::extraction_status_t status ) noexcept
			:	m_extracted{ extracted }
			,	m_handled{ handled }
			,	m_status{ status }
			{}

		//! Count of extracted messages.
		[[nodiscard]]
		std::size_t
		extracted() const noexcept { return m_extracted; }

		//! Count of handled messages.
		[[nodiscard]]
		std::size_t
		handled() const noexcept { return m_handled; }

		//! Extraction status (e.g. no messages, chain closed and so on).
		[[nodiscard]]
		mchain_props::extraction_status_t
		status() const noexcept { return m_status; }
	};

//
// mchain_send_result_t
//
/*!
 * \brief A result of attempt of sending messages to a message chain.
 *
 * This type plays the same role as mchain_receive_result_t but is used
 * for send operations.
 *
 * \since v.5.7.0
 */
class mchain_send_result_t
	{
		//! Count of messages sent.
		std::size_t m_sent;

		//! The status of send operation.
		mchain_props::push_status_t m_status;

	public:
		//! Default constructor.
		/*!
		 * Sets push_status_t::not_stored status.
		 */
		mchain_send_result_t() noexcept
			:	m_sent{ 0u }
			,	m_status{ mchain_props::push_status_t::not_stored }
			{}

		//! Initializing constructor.
		mchain_send_result_t(
			std::size_t sent,
			mchain_props::push_status_t status )
			:	m_sent{ sent }
			,	m_status{ status }
			{}
			
		//! Count of messages sent.
		[[nodiscard]]
		std::size_t
		sent() const noexcept { return m_sent; }

		//! Status of send operation.
		[[nodiscard]]
		mchain_props::push_status_t
		status() const noexcept { return m_status; }
	};

namespace mchain_props {

//
// msg_count_status_t
//
/*!
 * \brief Status of limit for messages to be extracted/handled
 * during a bulk operation on a mchain.
 *
 * \since v.5.6.0
 */
enum class msg_count_status_t
	{
		//! Message count limit is not set yet.
		undefined,
		//! Message count limit is set.
		defined
	};

namespace details {

//
// bulk_processing_basic_data_t
//
struct bulk_processing_basic_data_t
	{
		//! Type of stop-predicate.
		/*!
		 * Must return \a true if receive procedure should be stopped.
		 */
		using stop_predicate_t = std::function< bool() >;

		//! Type of chain-closed event.
		using chain_closed_handler_t = std::function< void(const mchain_t &) >;

		//! Minimal count of messages to be extracted.
		/*!
		 * Value 0 means that this parameter is not set.
		 */
		std::size_t m_to_extract = { 0 };
		//! Minimal count of messages to be handled.
		/*!
		 * Value 0 means that this parameter it not set.
		 */
		std::size_t m_to_handle = { 0 };

		//! Timeout for waiting on empty queue.
		mchain_props::duration_t m_empty_timeout =
				{ mchain_props::details::infinite_wait_special_timevalue() };

		//! Total time for all work of advanced receive.
		mchain_props::duration_t m_total_time =
				{ mchain_props::details::infinite_wait_special_timevalue() };

		//! Optional stop-predicate.
		stop_predicate_t m_stop_predicate;

		//! Optional chain-closed handler.
		chain_closed_handler_t m_chain_closed_handler;
	};

//
// mchain_bulk_processing_basic_params_t
//
template< typename Basic_Data >
class mchain_bulk_processing_basic_params_t
	{
	public :
		//! Type of stop-predicate.
		using stop_predicate_t = typename Basic_Data::stop_predicate_t;

		//! Type of chain-closed event.
		using chain_closed_handler_t = typename Basic_Data::chain_closed_handler_t;

	private :
		Basic_Data m_data;

	protected :
		//! Set limit for count of messages to be extracted.
		void
		set_extract_n( std::size_t v ) noexcept
			{
				m_data.m_to_extract = v;
			}

		//! Set limit for count of messages to be handled.
		void
		set_handle_n( std::size_t v ) noexcept
			{
				m_data.m_to_handle = v;
			}

		//! Set timeout for waiting on empty chain.
		/*!
		 * \note This value will be ignored if total_time() is also used
		 * to set total receive time.
		 *
		 * \note Argument \a v can be of type duration_t or
		 * so_5::infinite_wait or so_5::no_wait.
		 */
		template< typename Timeout >
		void
		set_empty_timeout( Timeout v ) noexcept
			{
				m_data.m_empty_timeout = mchain_props::details::actual_timeout( v );
			}

		//! Set total time for the whole receive operation.
		/*!
		 * \note Argument \a v can be of type duration_t or
		 * so_5::infinite_wait or so_5::no_wait.
		 */
		template< typename Timeout >
		void
		set_total_time( Timeout v ) noexcept
			{
				m_data.m_total_time = mchain_props::details::actual_timeout( v );
			}

		//! Set user condition for stopping receive operation.
		/*!
		 * \note \a predicate should return \a true if receive must
		 * be stopped.
		 */
		void
		set_stop_on( stop_predicate_t predicate ) noexcept
			{
				m_data.m_stop_predicate = std::move(predicate);
			}

		//! Set handler for chain-closed event.
		/*!
		 * If there is a previously set handler the old handler will be lost.
		 */
		void
		set_on_close( chain_closed_handler_t handler ) noexcept
			{
				m_data.m_chain_closed_handler = std::move(handler);
			}

	public :
		//! Default constructor.
		mchain_bulk_processing_basic_params_t() = default;

		//! Initializing constructor.
		mchain_bulk_processing_basic_params_t( Basic_Data data )
			:	m_data{ std::move(data) }
			{}

		//! Get limit for count of messages to be extracted.
		std::size_t
		to_extract() const noexcept { return m_data.m_to_extract; }

		//! Get limit for count of message to be handled.
		std::size_t
		to_handle() const noexcept { return m_data.m_to_handle; }

		//! Get timeout for waiting on empty chain.
		const mchain_props::duration_t &
		empty_timeout() const noexcept { return m_data.m_empty_timeout; }

		//! Get total time for the whole receive operation.
		const mchain_props::duration_t &
		total_time() const noexcept { return m_data.m_total_time; }

		//! Get user condition for stopping receive operation.
		const stop_predicate_t &
		stop_on() const noexcept
			{
				return m_data.m_stop_predicate;
			}

		//! Get handler for chain-closed event.
		const chain_closed_handler_t &
		closed_handler() const noexcept
			{
				return m_data.m_chain_closed_handler;
			}

		//! Access to internal data.
		const auto &
		so5_data() const noexcept { return m_data; }
	};

} /* namespace details */

} /* namespace mchain_props */

//
// mchain_bulk_processing_params_t
//
/*!
 * \brief Basic parameters for advanced receive from %mchain and for
 * multi chain select.
 *
 * \since v.5.5.16
 */
template< typename Data, typename Derived >
class mchain_bulk_processing_params_t
	:	public mchain_props::details::mchain_bulk_processing_basic_params_t< Data >
	{
		using basic_t =
				mchain_props::details::mchain_bulk_processing_basic_params_t<Data>;

	public :
		using actual_type = Derived;

		using data_type = Data;

	protected :
		//! Helper method to get a reference to itself but with a
		//! type of the derived class.
		actual_type &
		self_reference() { return static_cast< actual_type & >(*this); }

		//! Helper method to make a clone with msg_count_status_t::defined
		//! status.
		decltype(auto)
		clone_as_defined() noexcept
			{
				return self_reference().template so5_clone_if_necessary<
						mchain_props::msg_count_status_t::defined >();
			}

	public :
		//! Default constructor.
		mchain_bulk_processing_params_t() = default;

		//! Initializing constructor.
		mchain_bulk_processing_params_t( Data data )
			:	basic_t{ std::move(data) }
			{}

		//! A directive to handle all messages until chain will be closed
		//! or receiving will be stopped manually.
		/*!
		 * Usage example:
		 * \code
		 * so_5::receive(so_5::from(ch).handle_all(), ...);
		 * \endcode
		 *
		 * \since v.5.6.0
		 */
		decltype(auto)
		handle_all() noexcept
			{
				this->set_handle_n( 0u );
				return clone_as_defined();
			}

		//! Set limit for count of messages to be extracted.
		/*!
		 * When extract_n() is used then receive() will be finished
		 * after extraction of the specified number of message.
		 * 
		 * Usage example:
		 * \code
		 * so_5::receive(so_5::from(ch).extract_n(2), ...);
		 * \endcode
		 */
		decltype(auto)
		extract_n( std::size_t v ) noexcept
			{
				this->set_extract_n( v );
				return clone_as_defined();
			}

		//! Set limit for count of messages to be handled.
		/*!
		 * When handled_n() is used then receive() will be finished
		 * after handling of the specified number of message.
		 * 
		 * Usage example:
		 * \code
		 * so_5::receive(so_5::from(ch).handle_n(2), ...);
		 * \endcode
		 */
		decltype(auto)
		handle_n( std::size_t v ) noexcept
			{
				this->set_handle_n( v );
				return clone_as_defined();
			}

		//! Set timeout for waiting on empty chain.
		/*!
		 * \note This value will be ignored if total_time() is also used
		 * to set total receive time.
		 *
		 * \note Argument \a v can be of type duration_t or
		 * so_5::infinite_wait or so_5::no_wait.
		 */
		template< typename Timeout >
		actual_type &
		empty_timeout( Timeout v ) noexcept
			{
				this->set_empty_timeout( std::move(v) );
				return self_reference();
			}

		using basic_t::empty_timeout;

		/*!
		 * \brief Disable waiting on the empty queue.
		 *
		 * \par Usage example:
		 * \code
			so_5::mchain_t ch = env.create_mchain(...);
			receive( from(ch).no_wait_on_empty(), ... );
		 * \endcode
		 *
		 * \note It is just a shorthand for:
		 * \code
			receive( from(chain).empty_timeout(std::chrono::seconds(0)), ...);
		 * \endcode
		 */
		actual_type &
		no_wait_on_empty() noexcept
			{
				return empty_timeout(
						mchain_props::details::no_wait_special_timevalue() );
			}

		//! Set total time for the whole receive operation.
		/*!
		 * \note Argument \a v can be of type duration_t or
		 * so_5::infinite_wait or so_5::no_wait.
		 */
		template< typename Timeout >
		actual_type &
		total_time( Timeout v ) noexcept
			{
				this->set_total_time( std::move(v) );
				return self_reference();
			}

		using basic_t::total_time;

		//! Set user condition for stopping receive operation.
		/*!
		 * \note \a predicate should return \a true if receive must
		 * be stopped.
		 */
		actual_type &
		stop_on( typename basic_t::stop_predicate_t predicate ) noexcept
			{
				this->set_stop_on( std::move(predicate) );
				return self_reference();
			}

		using basic_t::stop_on;

		//! Set handler for chain-closed event.
		/*!
		 * If there is a previously set handler the old handler will be lost.
		 *
		 * Usage example:
		 * \code
		 * so_5::mchain_t ch1 = so_5::create_mchain(...);
		 * so_5::mchain_t ch2 = so_5::create_mchain(...);
		 * ...
		 * // Stop reading channels when any of channels is closed. 
		 * bool some_ch_closed = false;
		 * so_5::select(
		 * 	so_5::from_all()
		 * 		.handle_all()
		 * 		.on_close([&some_ch_closed](const so_5::mchain_t &) {
		 * 				some_ch_closed = true;
		 * 			})
		 * 		.stop_on([&some_ch_closed]{ return some_ch_closed; }),
		 * 	receive_case(ch1, ...)
		 * 	receive_case(ch2, ...)
		 * 	...);
		 * \endcode
		 *
		 * \since v.5.5.17
		 */
		actual_type &
		on_close( typename basic_t::chain_closed_handler_t handler ) noexcept
			{
				this->set_on_close( std::move(handler) );
				return self_reference();
			}
	};

namespace mchain_props {

namespace details {

//
// adv_receive_data_t
//
/*!
 * \brief Container of parameters for receive() function.
 *
 * \since v.5.6.0
 */
struct adv_receive_data_t : public bulk_processing_basic_data_t
	{
		//! A chain to be used in receive operation.
		mchain_t m_chain;

		//! Default constructor.
		adv_receive_data_t() = default;

		//! Initializing constructor.
		adv_receive_data_t( mchain_t chain )
			:	m_chain{ std::move(chain) }
			{}
	};

} /* namespace details */

} /* namespace mchain_props */

//
// mchain_receive_params_t
//
/*!
 * \brief Parameters for advanced receive from %mchain.
 *
 * \sa so_5::from().
 *
 * \note Derived from basic_receive_params_t since v.5.5.16.
 *
 * \tparam Msg_Count_Status status of message count limit.
 *
 * \since v.5.5.13
 */
template< mchain_props::msg_count_status_t Msg_Count_Status >
class mchain_receive_params_t final
	: public mchain_bulk_processing_params_t<
	  		mchain_props::details::adv_receive_data_t,
	  		mchain_receive_params_t< Msg_Count_Status > >
	{
		//! Short alias for base type.
		using base_type = mchain_bulk_processing_params_t<
				mchain_props::details::adv_receive_data_t,
				mchain_receive_params_t< Msg_Count_Status > >;

	public :
		//! Make of clone with different Msg_Count_Status or return
		//! a reference to the same object.
		template< mchain_props::msg_count_status_t New_Msg_Count_Status >
		[[nodiscard]]
		decltype(auto)
		so5_clone_if_necessary() noexcept
			{
				if constexpr( New_Msg_Count_Status != Msg_Count_Status )
					return mchain_receive_params_t< New_Msg_Count_Status >{
							this->so5_data()
						};
				else
					return *this;
			}

		//! Initializing constructor.
		mchain_receive_params_t(
			//! Chain from which messages must be extracted and handled.
			mchain_t chain )
			:	base_type{ typename base_type::data_type{ std::move(chain) } }
			{}

		//! Initializing constructor for the case of cloning.
		mchain_receive_params_t(
			typename base_type::data_type data )
			:	base_type{ std::move(data) }
			{}

		//! Chain from which messages must be extracted and handled.
		const mchain_t &
		chain() const { return this->so5_data().m_chain; }
	};

//
// from
//
/*!
 * \brief A helper function for simplification of creation of %mchain_receive_params instance.
 *
 * \attention
 * Since v.5.6.0 at least handle_all(), handle_n() or extract_n() should be
 * called before passing result of from() to receive() function.
 *
 * \par Usage examples:
	\code
	so_5::mchain_t chain = env.create_mchain(...);

	// Receive and handle 3 messages.
	// If there is no 3 messages in the mchain the receive will wait infinitely.
	// A return from receive will be after handling of 3 messages or
	// if the mchain is closed explicitely.
	receive( from(chain).handle_n( 3 ),
			handlers... );

	// Receive and handle 3 messages.
	// If there is no 3 messages in the mchain the receive will wait
	// no more that 200ms.
	// A return from receive will be after handling of 3 messages or
	// if the mchain is closed explicitely, or if there is no messages
	// for more than 200ms.
	receive( from(chain).handle_n( 3 ).empty_timeout( milliseconds(200) ),
			handlers... );

	// Receive all messages from the chain.
	// If there is no message in the chain then wait no more than 500ms.
	// A return from receive will be after explicit close of the chain
	// or if there is no messages for more than 500ms.
	receive( from(chain).handle_all().empty_timeout( milliseconds(500) ),
			handlers... );

	// Receve any number of messages from the chain but do waiting and
	// handling for no more than 2s.
	receive( from(chain).handle_all().total_time( seconds(2) ),
			handlers... );

	// Receve 1000 messages from the chain but do waiting and
	// handling for no more than 2s.
	receive( from(chain).extract_n( 1000 ).total_time( seconds(2) ),
			handlers... );
	\endcode
 *
 * \since v.5.5.13
 */
inline mchain_receive_params_t< mchain_props::msg_count_status_t::undefined >
from( mchain_t chain )
	{
		return { std::move(chain) };
	}

namespace mchain_props {

namespace details {

//
// receive_actions_performer_t
//
/*!
 * \brief Helper class with implementation of main actions of
 * advanced receive operation.
 *
 * \since v.5.5.13
 */
template< typename Bunch >
class receive_actions_performer_t
	{
		const mchain_receive_params_t< msg_count_status_t::defined > & m_params;
		const Bunch & m_bunch;

		std::size_t m_extracted_messages = 0;
		std::size_t m_handled_messages = 0;
		extraction_status_t m_status;

	public :
		receive_actions_performer_t(
			const mchain_receive_params_t< msg_count_status_t::defined > & params,
			const Bunch & bunch )
			:	m_params( params )
			,	m_bunch( bunch )
			{}

		void
		handle_next( duration_t empty_timeout )
			{
				demand_t extracted_demand;
				m_status = m_params.chain()->extract(
						extracted_demand, empty_timeout );

				if( extraction_status_t::msg_extracted == m_status )
					{
						++m_extracted_messages;
						const bool handled = m_bunch.handle(
								extracted_demand.m_msg_type,
								extracted_demand.m_message_ref );
						if( handled )
							++m_handled_messages;
					}
				// Since v.5.5.17 we must check presence of chain-closed handler.
				// This handler must be used if chain is closed.
				else if( extraction_status_t::chain_closed == m_status )
					{
						if( const auto & handler = m_params.closed_handler() )
							so_5::details::invoke_noexcept_code(
								[&handler, this] {
									handler( m_params.chain() );
								} );
					}
			}

		extraction_status_t
		last_status() const { return m_status; }

		bool
		can_continue() const
			{
				if( extraction_status_t::chain_closed == m_status )
					return false;

				if( m_params.to_handle() &&
						m_handled_messages >= m_params.to_handle() )
					return false;

				if( m_params.to_extract() &&
						m_extracted_messages >= m_params.to_extract() )
					return false;

				if( m_params.stop_on() && m_params.stop_on()() )
					return false;

				return true;
			}

		mchain_receive_result_t
		make_result() const
			{
				return mchain_receive_result_t{
						m_extracted_messages,
						m_handled_messages,
						m_extracted_messages ? extraction_status_t::msg_extracted :
								m_status
					};
			}
	};

/*!
 * \brief An implementation of advanced receive when a limit for total
 * operation time is defined.
 *
 * \since v.5.5.13
 */
template< typename Bunch >
inline mchain_receive_result_t
receive_with_finite_total_time(
	const mchain_receive_params_t<msg_count_status_t::defined> & params,
	const Bunch & bunch )
	{
		receive_actions_performer_t< Bunch > performer( params, bunch );

		so_5::details::remaining_time_counter_t remaining_time(
				params.total_time() );
		do
			{
				// Since v.5.8.4 we have to take the empty_timeout value
				// into the account.
				const duration_t this_iteration_wait_time = std::min(
						remaining_time.remaining(),
						params.empty_timeout() );

				performer.handle_next( this_iteration_wait_time );
				if( !performer.can_continue() )
					break;
				remaining_time.update();
			}
		while( remaining_time );

		return performer.make_result();
	}

/*!
 * \brief An implementation of advanced receive when there is no
 * limit for total operation time is defined.
 *
 * \since v.5.5.13
 */
template< typename Bunch >
inline mchain_receive_result_t
receive_without_total_time(
	const mchain_receive_params_t<msg_count_status_t::defined> & params,
	const Bunch & bunch )
	{
		receive_actions_performer_t< Bunch > performer{ params, bunch };

		do
			{
				performer.handle_next( params.empty_timeout() );

				if( extraction_status_t::no_messages == performer.last_status() )
					// There is no need to continue.
					// This status means that empty_timeout has some value
					// and there is no any new message during empty_timeout.
					// And this means a condition for return from advanced
					// receive.
					break;
			}
		while( performer.can_continue() );

		return performer.make_result();
	}

/*!
 * \brief An implementation of main receive actions.
 *
 * \since v.5.5.17
 */
template< typename Bunch >
inline mchain_receive_result_t
perform_receive(
	const mchain_receive_params_t<msg_count_status_t::defined> & params,
	const Bunch & bunch )
	{
		if( !is_infinite_wait_timevalue( params.total_time() ) )
			return receive_with_finite_total_time( params, bunch );
		else
			return receive_without_total_time( params, bunch );
	}

} /* namespace details */

} /* namespace mchain_props */

//
// receve (advanced version)
//

/*!
 * \brief Advanced version of receive from %mchain.
 *
 * \attention It is an error if there are more than one handler for the
 * same message type in \a handlers.
 *
 * \attention
 * Since v.5.6.0 at least handle_all(), handle_n() or extract_n() should be
 * called before passing result of from() to receive() function.
 *
 * \par Usage examples:
	\code
	so_5::mchain_t chain = env.create_mchain(...);

	// Receive and handle 3 messages.
	// If there is no 3 messages in the mchain the receive will wait infinitely.
	// A return from receive will be after handling of 3 messages or
	// if the mchain is closed explicitely.
	receive( from(chain).handle_n( 3 ),
			[]( const first_message_type & msg ) { ... },
			[]( const second_message_type & msg ) { ... }, ... );

	// Receive and handle 3 messages.
	// If there is no 3 messages in the mchain the receive will wait
	// no more that 200ms.
	// A return from receive will be after handling of 3 messages or
	// if the mchain is closed explicitely, or if there is no messages
	// for more than 200ms.
	receive( from(chain).handle_n( 3 ).empty_timeout( milliseconds(200) ),
			[]( const first_message_type & msg ) { ... },
			[]( const second_message_type & msg ) { ... }, ... );

	// Receive all messages from the chain.
	// If there is no message in the chain then wait no more than 500ms.
	// A return from receive will be after explicit close of the chain
	// or if there is no messages for more than 500ms.
	receive( from(chain).handle_all().empty_timeout( milliseconds(500) ),
			[]( const first_message_type & msg ) { ... },
			[]( const second_message_type & msg ) { ... }, ... );

	// Receve any number of messages from the chain but do waiting and
	// handling for no more than 2s.
	receive( from(chain).handle_all().total_time( seconds(2) ),
			[]( const first_message_type & msg ) { ... },
			[]( const second_message_type & msg ) { ... }, ... );

	// Receve 1000 messages from the chain but do waiting and
	// handling for no more than 2s.
	receive( from(chain).extract_n( 1000 ).total_time( seconds(2) ),
			[]( const first_message_type & msg ) { ... },
			[]( const second_message_type & msg ) { ... }, ... );
	\endcode
 *
 * \par Handlers format examples:
 * \code
	receive( ch, so_5::infinite_wait,
		// Message instance by const reference.
		[]( const std::string & v ) {...},
		// Message instance by value (efficient for small types like int).
		[]( int v ) {...},
		// Message instance via mhood_t value.
		[]( so_5::mhood_t< some_message > v ) {...},
		// Message instance via const reference to mhood_t.
		[]( const so_5::mhood_t< some_another_message > & v ) {...},
		// Explicitly specified signal handler.
		so_5::handler< some_signal >( []{...} ),
		// Signal handler via mhood_t value.
		[]( so_5::mhood_t< some_another_signal > ) {...},
		// Signal handler via const reference to mhood_t.
		[]( const so_5::mhood_t< yet_another_signal > & ) {...} );
 * \endcode
 *
 * \since v.5.5.13
 */
template<
	mchain_props::msg_count_status_t Msg_Count_Status,
	typename... Handlers >
inline mchain_receive_result_t
receive(
	//! Parameters for receive.
	const mchain_receive_params_t<Msg_Count_Status> & params,
	//! Handlers for message processing.
	Handlers &&... handlers )
	{
		static_assert(
				Msg_Count_Status == mchain_props::msg_count_status_t::defined,
				"message count to be processed/extracted should be defined "
				"by using handle_all()/handle_n()/extract_n() methods" );

		using namespace so_5::details;
		using namespace so_5::mchain_props;
		using namespace so_5::mchain_props::details;

		handlers_bunch_t< sizeof...(handlers) > bunch;
		fill_handlers_bunch( bunch, 0,
				std::forward< Handlers >(handlers)... );

		return perform_receive( params, bunch );
	}

//
// prepared_receive_t
//
/*!
 * \brief Special container for holding receive parameters and receive cases.
 *
 * \note Instances of that type usually used without specifying the actual
 * type:
 * \code
	auto prepared = so_5::prepare_receive(
		from(ch).handle_n(10).empty_timeout(10s),
		some_handlers... );
	...
	auto r = so_5::receive( prepared );
 * \endcode
 * \note This is a moveable type, not copyable.
 *
 * \since v.5.5.17
 */
template< std::size_t Handlers_Count >
class prepared_receive_t
	{
		//! Parameters for receive.
		mchain_receive_params_t< mchain_props::msg_count_status_t::defined > m_params;

		//! Cases for receive.
		so_5::details::handlers_bunch_t< Handlers_Count > m_bunch;

	public :
		prepared_receive_t( const prepared_receive_t & ) = delete;
		prepared_receive_t &
		operator=( const prepared_receive_t & ) = delete;

		//! Initializing constructor.
		template< typename... Handlers >
		prepared_receive_t(
			mchain_receive_params_t< mchain_props::msg_count_status_t::defined > params,
			Handlers &&... cases )
			:	m_params( std::move(params) )
			{
				static_assert( sizeof...(Handlers) == Handlers_Count,
						"Handlers_count and sizeof...(Handlers) mismatch" );

				fill_handlers_bunch(
						m_bunch, 0u, std::forward<Handlers>(cases)... );
			}

		//! Move constructor.
		prepared_receive_t(
			prepared_receive_t && other )
			:	m_params( std::move(other.m_params) )
			,	m_bunch( std::move(other.m_bunch) )
			{}

		//! Move operator.
		prepared_receive_t &
		operator=( prepared_receive_t && other ) noexcept
			{
				prepared_receive_t tmp( std::move(other) );
				swap( &this, tmp );

				return *this;
			}

		//! Swap operation.
		friend void
		swap( prepared_receive_t & a, prepared_receive_t & b ) noexcept
			{
				using std::swap;

				swap( a.m_params, b.m_params );
				swap( a.m_bunch, b.m_bunch );
			}

		/*!
		 * \name Getters
		 * \{ 
		 */
		const auto &
		params() const noexcept { return m_params; }

		const auto &
		handlers() const noexcept { return m_bunch; }
		/*!
		 * \}
		 */
	};

//
// prepare_receive
//
/*!
 * \brief Create parameters for receive function to be used later.
 *
 * \attention
 * Since v.5.6.0 at least handle_all(), handle_n() or extract_n() should be
 * called before passing result of from() to prepare_receive() function.
 *
 * Accepts all parameters as advanced receive() version. For example:
 * \code
	// Receive and handle 3 messages.
	// If there is no 3 messages in the mchain the receive will wait
	// no more that 200ms.
	// A return from receive will be after handling of 3 messages or
	// if the mchain is closed explicitely, or if there is no messages
	// for more than 200ms.
	auto prepared1 = prepare_receive(
			from(chain).handle_n( 3 ).empty_timeout( milliseconds(200) ),
			[]( const first_message_type & msg ) { ... },
			[]( const second_message_type & msg ) { ... }, ... );

	// Receive all messages from the chain.
	// If there is no message in the chain then wait no more than 500ms.
	// A return from receive will be after explicit close of the chain
	// or if there is no messages for more than 500ms.
	auto prepared2 = prepare_receive(
			from(chain).handle_all().empty_timeout( milliseconds(500) ),
			[]( const first_message_type & msg ) { ... },
			[]( const second_message_type & msg ) { ... }, ... );
 * \endcode
 *
 * \since v.5.5.17
 */
template<
	mchain_props::msg_count_status_t Msg_Count_Status,
	typename... Handlers >
prepared_receive_t< sizeof...(Handlers) >
prepare_receive(
	//! Parameters for advanced receive.
	const mchain_receive_params_t< Msg_Count_Status > & params,
	//! Handlers
	Handlers &&... handlers )
	{
		static_assert(
				Msg_Count_Status == mchain_props::msg_count_status_t::defined,
				"message count to be processed/extracted should be defined "
				"by using handle_all()/handle_n()/extract_n() methods" );

		return prepared_receive_t< sizeof...(Handlers) >(
				params,
				std::forward<Handlers>(handlers)... );
	}

/*!
 * \brief A receive operation to be done on previously prepared receive params.
 *
 * Usage of ordinary forms of receive() functions inside loops could be
 * inefficient because of wasting resources on constructions of internal
 * objects with descriptions of handlers on each receive() call.  More
 * efficient way is preparation of all receive params and reusing them later. A
 * combination of so_5::prepare_receive() and so_5::receive(prepared_receive_t)
 * allows to do that.
 *
 * Usage example:
 * \code
	auto prepared = so_5::prepare_receive(
		so_5::from(ch).extract_n(10).empty_timeout(200ms),
		some_handlers... );
	...
	while( !some_condition )
	{
		auto r = so_5::receive( prepared );
		...
	}
 * \endcode
 *
 * \since v.5.5.17
 */
template< std::size_t Handlers_Count >
mchain_receive_result_t
receive(
	const prepared_receive_t< Handlers_Count > & prepared )
	{
		return mchain_props::details::perform_receive(
				prepared.params(),
				prepared.handlers() );
	}

} /* namespace so_5 */

