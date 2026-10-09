/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0

	\brief Event-related stuff.
*/

#if !defined( SO_5_EXECUTION_DEMAND_HPP )
#define SO_5_EXECUTION_DEMAND_HPP

#include <so_5/types.hpp>
#include <so_5/current_thread_id.hpp>

#include <so_5/fwd.hpp>

#include <so_5/message.hpp>

#include <so_5/cpp_coro/task.hpp>

#include <so_5/exception.hpp>

#include <variant>

namespace so_5
{

//
// sync_event_handler_method_t
//
/*!
 * \brief Holder of a synchronous handler for a message.
 *
 * \since v.5.8.7
 */
using sync_event_handler_method_t = std::function< void(message_ref_t &) >;

//FIXME: maybe it has sense to do that:
//[[deprecated]]
//using event_handler_method_t = sync_event_handler_method_t;

//
// async_event_handler_method_t
//
/*!
 * \brief Holder of asynchronous handler for a message.
 *
 * \since v.5.8.7
 */
using async_event_handler_method_t = std::function<
		so_5::cpp_coro::task_t<void>(message_ref_t) >;

//
// event_handler_holder_t
//
/*!
 * \brief Type of holder of a handler for a message.
 *
 * This type can hold different types of handlers (synchronous
 * and asynchronous).
 *
 * \since v.5.8.7
 */
using event_handler_holder_t = std::variant<
		sync_event_handler_method_t,
		async_event_handler_method_t
	>;

namespace low_level_api
{

//FIXME: document this!
/*!
 * \since v.5.8.7
 */
[[nodiscard]] inline
const sync_event_handler_method_t &
query_ref_to_sync_handler(
	const event_handler_holder_t & holder )
	{
		const auto * handler = std::get_if<sync_event_handler_method_t>(
				std::addressof(holder) );
		if( !handler )
			SO_5_THROW_EXCEPTION( rc_sync_handler_expected,
					"synchronous event handler is expected, but asynchronous "
					"is found" );

		return *handler;
	}

//FIXME: document this!
/*!
 * \since v.5.8.7
 */
[[nodiscard]] inline
const async_event_handler_method_t &
query_ref_to_async_handler(
	const event_handler_holder_t & holder )
	{
		const auto * handler = std::get_if<async_event_handler_method_t>(
				std::addressof(holder) );
		if( !handler )
			{
				SO_5_THROW_EXCEPTION( rc_async_handler_expected,
						"asynchronous event handler is expected, but synchronous "
						"is found" );
			}

		return *handler;
	}

//FIXME: document this!
/*!
 * \since v.5.8.7
 */
inline void
invoke_sync_event_handler(
	const event_handler_holder_t & holder,
	message_ref_t & msg )
	{
		query_ref_to_sync_handler( holder )( msg );
	}

//FIXME: document this!
/*!
 * \since v.5.8.7
 */
inline
so_5::cpp_coro::task_t< void >
invoke_async_event_handler(
	const event_handler_holder_t & holder,
	message_ref_t msg )
	{
		return (query_ref_to_async_handler( holder )( std::move(msg) ));
	}

//FIXME: document this!
/*!
 * \since v.5.8.7
 */
class any_handler_as_async_invoker_t
	{
		/// Message to be passed to the handler.
		message_ref_t & m_msg_ref;

	public :
		any_handler_as_async_invoker_t(
			message_ref_t & msg_ref )
			: m_msg_ref{ msg_ref }
			{}

		so_5::cpp_coro::task_t< void >
		operator()( const sync_event_handler_method_t & sync_handler )
			{
				sync_handler( m_msg_ref );

				co_return;
			}

		so_5::cpp_coro::task_t< void >
		operator()( const async_event_handler_method_t & async_handler )
			{
				co_return co_await async_handler( std::move(m_msg_ref) );
			}
	};

} /* namespace low_level_api */

struct execution_demand_t;

//
// demand_handler_pfn_t
//
/*!
 * \since
 * v.5.2.0
 *
 * \brief Demand handler prototype.
 */
using demand_handler_pfn_t = void (*)(
	current_thread_id_t,
	execution_demand_t & );

//
// execution_demand_t
//
/*!
 * \since
 * v.5.4.0
 *
 * \brief A description of event execution demand.
 */
struct execution_demand_t
{
	//! Receiver of demand.
	agent_t * m_receiver;
	//! Optional message limit for that message.
	const message_limit::control_block_t * m_limit;
	//! ID of mbox.
	mbox_id_t m_mbox_id;
	//! Type of the message.
	std::type_index m_msg_type;
	//! Event incident.
	message_ref_t m_message_ref;
	//! Demand handler.
	demand_handler_pfn_t m_demand_handler;

	//! Default constructor.
	execution_demand_t() noexcept
		:	m_receiver( nullptr )
		,	m_limit( nullptr )
		,	m_mbox_id( 0 )
		,	m_msg_type( typeid(void) )
		,	m_demand_handler( nullptr )
		{}

	execution_demand_t(
		agent_t * receiver,
		const message_limit::control_block_t * limit,
		mbox_id_t mbox_id,
		std::type_index msg_type,
		message_ref_t message_ref,
		demand_handler_pfn_t demand_handler ) noexcept
		:	m_receiver( receiver )
		,	m_limit( limit )
		,	m_mbox_id( mbox_id )
		,	m_msg_type( msg_type )
		,	m_message_ref( std::move( message_ref ) )
		,	m_demand_handler( demand_handler )
		{}

	/*!
	 * \since
	 * v.5.5.8
	 *
	 * \brief Helper method to simplify demand execution.
	 */
	inline void
	call_handler( current_thread_id_t thread_id )
		{
			(*m_demand_handler)( thread_id, *this );
		}
};

//
// execution_hint_t
//
/*!
 * \brief A hint for a dispatcher for execution of event
 * for the concrete execution_demand.
 *
 * An instance of execution hint can be in two "states":
 *
 * - holds actual event handler. In that case an invocation of
 *   exec() method will decrement message limit counter and then
 *   will call the event handler;
 * - empty. In that case an invocation of exec() method will only
 *   decrement message limit counter and nothing more.
 *
 * \since v.5.4.0
 */
class execution_hint_t
{
public :
	//! Type of function for calling event handler directly.
	using direct_func_t = std::function<
				void( execution_demand_t &, current_thread_id_t ) >;

	//! Initializing constructor.
	execution_hint_t(
		execution_demand_t & demand,
		direct_func_t direct_func,
		thread_safety_t thread_safety )
		:	m_demand( demand )
		,	m_direct_func( std::move( direct_func ) )
		,	m_thread_safety( thread_safety )
		{}

	//! Call event handler directly.
	void
	exec( current_thread_id_t working_thread_id ) const
		{
			// If message limit is defined then message count
			// must be decremented.
			message_limit::control_block_t::decrement( m_demand.m_limit );

			// Now demand can be handled.
			if( m_direct_func )
				m_direct_func( m_demand, is_thread_safe() ?
						null_current_thread_id() : working_thread_id );
		}

	//! Is thread safe handler?
	bool
	is_thread_safe() const
		{
			return thread_safe == m_thread_safety;
		}

	//! Create execution_hint object for the case when
	//! event handler not found.
	/*!
	 * This hint is necessary only for decrementing the counter of
	 * messages if message limit is used for the message to be processed.
	 */
	static execution_hint_t
	create_empty_execution_hint( execution_demand_t & demand )
		{
			return execution_hint_t( demand );
		}

private :
	//! A reference to demand for which that hint has been created.
	execution_demand_t & m_demand;

	//! Function for call event handler directly.
	direct_func_t m_direct_func;

	//! Thread safety for event handler.
	thread_safety_t m_thread_safety;

	//! A special constructor for the case when there is no
	//! handler for message.
	execution_hint_t( execution_demand_t & demand )
		:	m_demand( demand )
		,	m_direct_func()
		,	m_thread_safety( thread_safe )
		{}

// Only for the unit-testing purposes!
#if defined( SO_5_EXECUTION_HINT_UNIT_TEST )
public :
	//! Is event handler defined for the demand?
	operator bool() const
		{
			return static_cast< bool >(m_direct_func);
		}
#endif
};

namespace details {

//
// msg_type_and_handler_pair_t
//
/*!
 * \brief Description of an event handler.
 *
 * \since v.5.5.13
 */
struct msg_type_and_handler_pair_t
	{
		//! Type of a message or signal.
		std::type_index m_msg_type;
		//! A handler for processing this message or signal.
		event_handler_holder_t m_handler;
		//! What message is expected by handler: mutable or immutable.
		/*!
		 * By default immutable message is expected.
		 *
		 * \since v.5.5.19
		 */
		message_mutability_t m_mutability;

		//! Default constructor.
		msg_type_and_handler_pair_t()
			:	m_msg_type{ typeid(void) }
			,	m_mutability{ message_mutability_t::immutable_message }
			{}
		//! Constructor for the case when only msg_type is known.
		/*!
		 * This constructor is intended for cases when
		 * msg_type_and_handler_pair_t instance is used
		 * as a key for searching in ordered sequences.
		 */
		msg_type_and_handler_pair_t(
			//! Type of a message or signal.
			std::type_index msg_type )
			:	m_msg_type{ std::move(msg_type) }
			,	m_mutability{ message_mutability_t::immutable_message }
			{}
		//! Initializing constructor.
		msg_type_and_handler_pair_t(
			//! Type of a message or signal.
			std::type_index msg_type,
			//! A handler for processing this message or signal.
			event_handler_holder_t handler,
			//! What message is expected by handler: mutable or immutable?
			message_mutability_t mutability )
			:	m_msg_type{ std::move(msg_type) }
			,	m_handler{ std::move(handler) }
			,	m_mutability{ mutability }
			{}
		//! Copy constructor.
		msg_type_and_handler_pair_t(
			const msg_type_and_handler_pair_t & o )
			:	m_msg_type{ o.m_msg_type }
			,	m_handler{ o.m_handler }
			,	m_mutability{ o.m_mutability }
			{}
		//! Move constructor.
		msg_type_and_handler_pair_t(
			msg_type_and_handler_pair_t && o ) noexcept
			:	m_msg_type{ std::move(o.m_msg_type) }
			,	m_handler{ std::move(o.m_handler) }
			,	m_mutability{ std::move(o.m_mutability) }
			{}

		//! Swap operation.
		friend void
		swap(
			msg_type_and_handler_pair_t & a,
			msg_type_and_handler_pair_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_msg_type, b.m_msg_type );
				swap( a.m_handler, b.m_handler );
				swap( a.m_mutability, b.m_mutability );
			}

		//! Copy operator.
		msg_type_and_handler_pair_t &
		operator=( const msg_type_and_handler_pair_t & o )
			{
				msg_type_and_handler_pair_t tmp{ o };
				swap( *this, tmp );
				return *this;
			}

		//! Move operator.
		msg_type_and_handler_pair_t &
		operator=( msg_type_and_handler_pair_t && o ) noexcept
			{
				msg_type_and_handler_pair_t tmp{ std::move(o) };
				swap( *this, tmp );
				return *this;
			}

//FIXME: should operator<=> be defined here instead of operator<?
		//! Comparison (strictly less than).
		bool
		operator<( const msg_type_and_handler_pair_t & o ) const
			{
				return m_msg_type < o.m_msg_type;
			}

		//! Comparison (strictly equal).
		bool
		operator==( const msg_type_and_handler_pair_t & o ) const
			{
				return m_msg_type == o.m_msg_type;
			}
	};

} /* namespace details */

} /* namespace so_5 */

#endif

