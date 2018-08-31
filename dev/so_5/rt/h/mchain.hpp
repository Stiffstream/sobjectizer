/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Public part of message chain related stuff.
 * \since
 * v.5.5.13
 */

#pragma once

#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/handler_makers.hpp>

#include <so_5/rt/h/fwd.hpp>

#include <so_5/details/h/invoke_noexcept_code.hpp>
#include <so_5/details/h/remaining_time_counter.hpp>

#include <chrono>
#include <functional>

namespace so_5 {

namespace mchain_props {

/*!
 * \since
 * v.5.5.13
 *
 * \brief An alias for type for repesenting timeout values.
 */
using duration_t = std::chrono::high_resolution_clock::duration;

namespace details {

//
// no_wait_special_timevalue
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Special value of %duration to indicate 'no_wait' case.
 */
inline duration_t
no_wait_special_timevalue() { return duration_t::zero(); }

//
// infinite_wait_special_timevalue
//
/*!
 * \since
 * v.5.5.13
 * 
 * \brief Special value of %duration to indicate 'infinite_wait' case.
 */
inline duration_t
infinite_wait_special_timevalue() { return duration_t::max(); }

//
// is_no_wait_timevalue
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Is time value means 'no_wait'?
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
 * \since
 * v.5.5.13
 *
 * \brief Is time value means 'infinite_wait'?
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
 * \since
 * v.5.5.13
 *
 * \brief Helper function for detection of actual value for waiting timeout.
 *
 * \note This helper implements convention that infinite waiting is
 * represented as duration_t::max() value.
 */
inline duration_t
actual_timeout( infinite_wait_indication )
	{
		return infinite_wait_special_timevalue();
	}

/*!
 * \since
 * v.5.5.13
 *
 * \brief Helper function for detection of actual value for waiting timeout.
 *
 * \note This helper implements convention that no waiting is
 * represented as duration_t::zero() value.
 */
inline duration_t
actual_timeout( no_wait_indication )
	{
		return no_wait_special_timevalue();
	}

/*!
 * \since
 * v.5.5.13
 *
 * \brief Helper function for detection of actual value for waiting timeout.
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
 * \since
 * v.5.5.13
 *
 * \brief Description of one demand in message chain.
 */
struct demand_t
	{
		//! Type of the message.
		std::type_index m_msg_type;
		//! Event incident.
		so_5::message_ref_t m_message_ref;
		//! Type of demand.
		so_5::invocation_type_t m_demand_type;

// NOTE: the full set of constructors and copy/move operators is defined
// because VC++12.0 doesn't generate move constructors/operators automatically
// and doesn't support '=default' construct.

		//! Default constructor.
		demand_t()
			:	m_msg_type( typeid(void) )
			,	m_demand_type( invocation_type_t::event )
			{}
		//! Initializing constructor.
		demand_t(
			std::type_index msg_type,
			so_5::message_ref_t message_ref,
			so_5::invocation_type_t demand_type )
			:	m_msg_type{ std::move(msg_type) }
			,	m_message_ref{ std::move(message_ref) }
			,	m_demand_type{ demand_type }
			{}

		//! Copy constructor.
		demand_t( const demand_t& o )
			:	m_msg_type{ o.m_msg_type }
			,	m_message_ref{ o.m_message_ref }
			,	m_demand_type{ o.m_demand_type }
			{}
		//! Move constructor.
		demand_t( demand_t && o )
			:	m_msg_type{ std::move(o.m_msg_type) }
			,	m_message_ref{ std::move(o.m_message_ref) }
			,	m_demand_type{ std::move(o.m_demand_type) }
			{}

		//! Swap operation.
		void
		swap( demand_t & o )
			{
				std::swap( m_msg_type, o.m_msg_type );
				m_message_ref.swap( o.m_message_ref );
				std::swap( m_demand_type, o.m_demand_type );
			}

		//! Copy operator.
		demand_t &
		operator=( const demand_t & o )
			{
				demand_t tmp{ o };
				tmp.swap( *this );
				return *this;
			}

		//! Move operator.
		demand_t &
		operator=( demand_t && o )
			{
				demand_t tmp{ std::move(o) };
				tmp.swap( *this );
				return *this;
			}
	};

//
// memory_usage_t
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Memory allocation for storage for size-limited chains.
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
 * \since
 * v.5.5.13
 *
 * \brief What reaction must be performed on attempt to push new message to
 * the full message chain.
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
 * \since
 * v.5.5.13
 *
 * \brief Parameters for defining chain size.
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
 * \since
 * v.5.5.13
 *
 * \brief Result of extraction of message from a message chain.
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
// close_mode_t
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief What to do with chain's content at close.
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
 * \since
 * v.5.5.13
 *
 * \brief Type of functor for notifies about arrival of a message to
 * the empty chain.
 *
 * \attention This function must be noexcept.
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
 * \since
 * v.5.5.13
 *
 * \brief An interace of message chain.
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
		virtual ~abstract_message_chain_t() SO_5_NOEXCEPT = default;

	public :
		using abstract_message_box_t::id;

		virtual mchain_props::extraction_status_t
		extract(
			//! Destination for extracted messages.
			mchain_props::demand_t & dest,
			//! Max time to wait on empty queue.
			mchain_props::duration_t empty_queue_timeout ) = 0;

		//! Cast message chain to message box.
		so_5::mbox_t
		as_mbox();

		//! Is message chain empty?
		virtual bool
		empty() const = 0;

		//! Count of messages in the chain.
		virtual std::size_t
		size() const = 0;

		//! Close the chain.
		virtual void
		close(
			//! What to do with chain's content.
			mchain_props::close_mode_t mode ) = 0;

		//! SObjectizer Environment for which the chain is created.
		virtual so_5::environment_t &
		environment() const = 0;

	protected :
		/*!
		 * \brief An extraction attempt as a part of multi chain select.
		 *
		 * \note In v.5.5.16 this method has an implementation. It is done to
		 * keep compatibility with previous version. This implementation throws
		 * an exception.
		 *
		 * \note This method is intended to be used by select_case_t.
		 *
		 * \since
		 * v.5.5.16
		 */
		virtual mchain_props::extraction_status_t
		extract(
			//! Destination for extracted messages.
			mchain_props::demand_t & dest,
			//! Select case to be stored for notification if mchain is empty.
			mchain_props::select_case_t & select_case );

		/*!
		 * \brief Removement of mchain from multi chain select.
		 *
		 * \note In v.5.5.16 this method has an implementation. It is done to
		 * keep compatibility with previous version. This implementation throws
		 * an exception.
		 *
		 * \note This method is intended to be used by select_case_t.
		 *
		 * \attention This method will be declared as pure virtual and noexcept
		 * in v.5.6.0.
		 *
		 * \since
		 * v.5.5.16
		 */
		virtual void
		remove_from_select(
			//! Select case to be removed from notification queue.
			mchain_props::select_case_t & select_case );
	};

//
// mchain_t
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Short name for smart pointer to message chain.
 */
using mchain_t = intrusive_ptr_t< abstract_message_chain_t >;

//
// close_drop_content
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Helper function for closing a message chain with dropping
 * all its content.
 *
 * \note Because of ADL it can be used without specifying namespaces.
 *
 * \par Usage example.
	\code
	so_5::mchain_t & ch = ...;
	... // Some work with chain.
	close_drop_content( ch );
	// Or:
	ch->close( so_5::mchain_props::close_mode_t::drop_content );
	\endcode
 */
inline void
close_drop_content( const mchain_t & ch )
	{
		ch->close( mchain_props::close_mode_t::drop_content );
	}

//
// close_retain_content
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Helper function for closing a message chain with retaining
 * all its content.
 *
 * \note Because of ADL it can be used without specifying namespaces.
 *
 * \par Usage example.
	\code
	so_5::mchain_t & ch = ...;
	... // Some work with chain.
	close_retain_content( ch );
	// Or:
	ch->close( so_5::mchain_props::close_mode_t::retain_content );
	\endcode
 */
inline void
close_retain_content( const mchain_t & ch )
	{
		ch->close( mchain_props::close_mode_t::retain_content );
	}

//
// mchain_params_t
//
/*!
 * \since
 * v.5.5.13
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
 * \since
 * v.5.5.13
 *
 * \brief Create parameters for size-unlimited %mchain.
 *
 * \par Usage example:
	\code
	so_5::environment_t & env = ...;
	auto chain = env.create_mchain( so_5::make_unlimited_mchain_params() );
	\endcode
 */
inline mchain_params_t
make_unlimited_mchain_params()
	{
		return mchain_params_t{ mchain_props::capacity_t::make_unlimited() };
	}

/*!
 * \since
 * v.5.5.13
 *
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
 * \since
 * v.5.5.13
 *
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
 * \since
 * v.5.5.13
 *
 * \brief A result of receive from %mchain.
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
		mchain_receive_result_t()
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
			mchain_props::extraction_status_t status )
			:	m_extracted{ extracted }
			,	m_handled{ handled }
			,	m_status{ status }
			{}

		//! Count of extracted messages.
		std::size_t
		extracted() const { return m_extracted; }

		//! Count of handled messages.
		std::size_t
		handled() const { return m_handled; }

		//! Extraction status (e.g. no messages, chain closed and so on).
		mchain_props::extraction_status_t
		status() const { return m_status; }
	};

//
// receive
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Receive and handle one message from message chain.
 *
 * \note Just one message will be extracted from the chain (if chain is not
 * empty). Then a handler for that message fill be searched in the \a handlers
 * list. If a handler is found then message will be passed to it. If not the
 * message will be discarded.
 *
 * \attention It is an error if there are more than one handler for the
 * same message type in \a handlers.
 *
 * \par Usage examples:
	\code
	so_5::environment_t & env = ...;
	so_5::mchain_t ch = env.create_mchain(...);

	// Extract and handle one message without waiting if the mchain is empty.
	auto r1 = receive(ch, so_5::no_wait, []( const my_message & m ) {...}, ...);

	// Extract and handle one message with infinite waiting if the mchain is empty.
	// The receive returns when message is extracted or mchain is closed.
	auto r2 = receive(ch, so_5::infinite_wait, ...);
	
	// Extract and handle one message with waiting no more than 200ms on empty mchain.
	auto r3 = receive(ch, std::chrono::milliseconds(200), ...);
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
 */
template< typename Timeout, typename... Handlers >
inline mchain_receive_result_t
receive(
	//! Message chain from which a message must be extracted.
	const so_5::mchain_t & chain,
	//! Maximum timeout for waiting for message on empty bag.
	Timeout waiting_timeout,
	//! Handlers for message processing.
	Handlers &&... handlers )
	{
		using namespace so_5::details;
		using namespace so_5::mchain_props;
		using namespace so_5::mchain_props::details;

		handlers_bunch_t< sizeof...(handlers) > bunch;
		fill_handlers_bunch( bunch, 0,
				std::forward< Handlers >(handlers)... );

		demand_t extracted_demand;
		const auto status = chain->extract(
				extracted_demand,
				actual_timeout( waiting_timeout ) );
		if( extraction_status_t::msg_extracted == status )
			{
				const bool handled = bunch.handle(
						extracted_demand.m_msg_type,
						extracted_demand.m_message_ref,
						extracted_demand.m_demand_type );

				return mchain_receive_result_t{ 1u, handled ? 1u : 0u, status };
			}

		return mchain_receive_result_t{ 0u, 0u, status };
	}

//
// mchain_bulk_processing_params_t
//
/*!
 * \brief Basic parameters for advanced receive from %mchain and for
 * multi chain select.
 *
 * \since
 * v.5.5.16
 */
template< typename Derived >
class mchain_bulk_processing_params_t
	{
	public :
		//! Actual type of params.
		using actual_type = Derived;

		//! Type of stop-predicate.
		/*!
		 * Must return \a true if receive procedure should be stopped.
		 */
		using stop_predicate = std::function< bool() >;

		//! Type of chain-closed event.
		/*!
		 * \since
		 * v.5.5.17
		 */
		using chain_closed_handler = std::function< void(const mchain_t &) >;

	protected :
		actual_type &
		self_reference() { return static_cast< actual_type & >(*this); }

	private :
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
		stop_predicate m_stop_predicate;

		//! Optional chain-closed handler.
		/*!
		 * \since
		 * v.5.5.17
		 */
		chain_closed_handler m_chain_closed_handler;

	public :
		//! Set limit for count of messages to be extracted.
		actual_type &
		extract_n( std::size_t v )
			{
				m_to_extract = v;
				return self_reference();
			}

		//! Get limit for count of messages to be extracted.
		std::size_t
		to_extract() const { return m_to_extract; }

		//! Set limit for count of messages to be handled.
		actual_type &
		handle_n( std::size_t v )
			{
				m_to_handle = v;
				return self_reference();
			}

		//! Get limit for count of message to be handled.
		std::size_t
		to_handle() const { return m_to_handle; }

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
		empty_timeout( Timeout v )
			{
				m_empty_timeout = mchain_props::details::actual_timeout( v );
				return self_reference();
			}

		//! Get timeout for waiting on empty chain.
		const mchain_props::duration_t &
		empty_timeout() const { return m_empty_timeout; }

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
		no_wait_on_empty()
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
		total_time( Timeout v )
			{
				m_total_time = mchain_props::details::actual_timeout( v );
				return self_reference();
			}

		//! Get total time for the whole receive operation.
		const mchain_props::duration_t &
		total_time() const { return m_total_time; }

		//! Set user condition for stopping receive operation.
		/*!
		 * \note \a predicate should return \a true if receive must
		 * be stopped.
		 */
		actual_type &
		stop_on( stop_predicate predicate )
			{
				m_stop_predicate = std::move(predicate);
				return self_reference();
			}

		//! Get user condition for stopping receive operation.
		const stop_predicate &
		stop_on() const
			{
				return m_stop_predicate;
			}

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
		 * 		.on_close([&some_ch_closed](const so_5::mchain_t &) {
		 * 				some_ch_closed = true;
		 * 			})
		 * 		.stop_on([&some_ch_closed]{ return some_ch_closed; }),
		 * 	case_(ch1, ...)
		 * 	case_(ch2, ...)
		 * 	...);
		 * \endcode
		 *
		 * \since
		 * v.5.5.17
		 */
		actual_type &
		on_close( chain_closed_handler handler )
			{
				m_chain_closed_handler = std::move(handler);
				return self_reference();
			}

		//! Get handler for chain-closed event.
		/*!
		 * \since
		 * v.5.5.17
		 */
		const chain_closed_handler &
		closed_handler() const
			{
				return m_chain_closed_handler;
			}
	};

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
 * \since
 * v.5.5.13
 */
class mchain_receive_params_t final
	: public mchain_bulk_processing_params_t< mchain_receive_params_t >
	{
		//! Chain from which messages must be extracted and handled.
		mchain_t m_chain;

	public :
		//! Initializing constructor.
		mchain_receive_params_t(
			//! Chain from which messages must be extracted and handled.
			mchain_t chain )
			:	m_chain{ std::move(chain) }
			{}

		//! Chain from which messages must be extracted and handled.
		const mchain_t &
		chain() const { return m_chain; }
	};

//
// from
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief A helper function for simplification of creation of %mchain_receive_params instance.
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
	receive( from(chain).empty_timeout( milliseconds(500) ),
			handlers... );

	// Receve any number of messages from the chain but do waiting and
	// handling for no more than 2s.
	receive( from(chain).total_time( seconds(2) ),
			handlers... );

	// Receve 1000 messages from the chain but do waiting and
	// handling for no more than 2s.
	receive( from(chain).extract_n( 1000 ).total_time( seconds(2) ),
			handlers... );
	\endcode
 */
inline mchain_receive_params_t
from( mchain_t chain )
	{
		return mchain_receive_params_t{ std::move(chain) };
	}

namespace mchain_props {

namespace details {

//
// receive_actions_performer_t
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Helper class with implementation of main actions of
 * advanced receive operation.
 */
template< typename Bunch >
class receive_actions_performer_t
	{
		const mchain_receive_params_t & m_params;
		const Bunch & m_bunch;

		std::size_t m_extracted_messages = 0;
		std::size_t m_handled_messages = 0;
		extraction_status_t m_status;

	public :
		receive_actions_performer_t(
			const mchain_receive_params_t & params,
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
								extracted_demand.m_message_ref,
								extracted_demand.m_demand_type );
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
 * \since
 * v.5.5.13
 *
 * \brief An implementation of advanced receive when a limit for total
 * operation time is defined.
 */
template< typename Bunch >
inline mchain_receive_result_t
receive_with_finite_total_time(
	const mchain_receive_params_t & params,
	const Bunch & bunch )
	{
		receive_actions_performer_t< Bunch > performer( params, bunch );

		so_5::details::remaining_time_counter_t remaining_time(
				params.total_time() );
		do
			{
				performer.handle_next( remaining_time.remaining() );
				if( !performer.can_continue() )
					break;
				remaining_time.update();
			}
		while( remaining_time );

		return performer.make_result();
	}

/*!
 * \since
 * v.5.5.13
 *
 * \brief An implementation of advanced receive when there is no
 * limit for total operation time is defined.
 */
template< typename Bunch >
inline mchain_receive_result_t
receive_without_total_time(
	const mchain_receive_params_t & params,
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
 * \since
 * v.5.5.17
 */
template< typename Bunch >
inline mchain_receive_result_t
perform_receive(
	const mchain_receive_params_t & params,
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
 * \since
 * v.5.5.13
 *
 * \brief Advanced version of receive from %mchain.
 *
 * \attention It is an error if there are more than one handler for the
 * same message type in \a handlers.
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
	receive( from(chain).empty_timeout( milliseconds(500) ),
			[]( const first_message_type & msg ) { ... },
			[]( const second_message_type & msg ) { ... }, ... );

	// Receve any number of messages from the chain but do waiting and
	// handling for no more than 2s.
	receive( from(chain).total_time( seconds(2) ),
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
 */
template< typename... Handlers >
inline mchain_receive_result_t
receive(
	//! Parameters for receive.
	const mchain_receive_params_t & params,
	//! Handlers for message processing.
	Handlers &&... handlers )
	{
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
 * \since
 * v.5.5.17
 */
template< std::size_t Handlers_Count >
class prepared_receive_t
	{
		//! Parameters for receive.
		mchain_receive_params_t m_params;

		//! Cases for receive.
		so_5::details::handlers_bunch_t< Handlers_Count > m_bunch;

	public :
		prepared_receive_t( const prepared_receive_t & ) = delete;
		prepared_receive_t &
		operator=( const prepared_receive_t & ) = delete;

		//! Initializing constructor.
		template< typename... Handlers >
		prepared_receive_t(
			mchain_receive_params_t params,
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
		operator=( prepared_receive_t && other ) SO_5_NOEXCEPT
			{
				prepared_receive_t tmp( std::move(other) );
				this->swap(tmp);
				return *this;
			}

		//! Swap operation.
		void
		swap( prepared_receive_t & o ) SO_5_NOEXCEPT
			{
				std::swap( o.m_params, o.m_params );
				m_bunch.swap( o.m_bunch );
			}

		/*!
		 * \name Getters
		 * \{ 
		 */
		const mchain_receive_params_t &
		params() const { return m_params; }

		const so_5::details::handlers_bunch_t< Handlers_Count > &
		handlers() const { return m_bunch; }
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
			from(chain).empty_timeout( milliseconds(500) ),
			[]( const first_message_type & msg ) { ... },
			[]( const second_message_type & msg ) { ... }, ... );
 * \endcode
 *
 * \since
 * v.5.5.17
 */
template< typename... Handlers >
prepared_receive_t< sizeof...(Handlers) >
prepare_receive(
	//! Parameters for advanced receive.
	const mchain_receive_params_t & params,
	//! Handlers
	Handlers &&... handlers )
	{
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
 * \since
 * v.5.5.17
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

