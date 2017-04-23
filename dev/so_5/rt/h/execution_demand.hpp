/*
	SObjectizer 5.
*/

/*!
	\file
	\since
	v.5.4.0

	\brief Event-related stuff.
*/

#pragma once

#include <so_5/h/types.hpp>
#include <so_5/h/current_thread_id.hpp>

#include <so_5/rt/h/fwd.hpp>

#include <so_5/rt/h/message.hpp>

namespace so_5
{

//
// event_handler_method_t
//
/*!
 * \since
 * v.5.3.0
 *
 * \brief Type of event handler method.
 */
typedef std::function< void(invocation_type_t, message_ref_t &) >
		event_handler_method_t;

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
typedef void (*demand_handler_pfn_t)(
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
	execution_demand_t()
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
		demand_handler_pfn_t demand_handler )
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
 * \since
 * v.5.4.0
 *
 * \brief A hint for a dispatcher for execution of event
 * for the concrete execution_demand.
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
 * \since
 * v.5.5.13
 *
 * \brief Description of an event handler.
 */
struct msg_type_and_handler_pair_t
	{
		//! Type of a message or signal.
		std::type_index m_msg_type;
		//! A handler for processing this message or signal.
		event_handler_method_t m_handler;
		//! What message is expected by handler: mutable or immutable.
		/*!
		 * By default immutable message is expected.
		 *
		 * \since
		 * v.5.5.19
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
			event_handler_method_t handler,
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
			msg_type_and_handler_pair_t && o ) SO_5_NOEXCEPT
			:	m_msg_type{ std::move(o.m_msg_type) }
			,	m_handler{ std::move(o.m_handler) }
			,	m_mutability{ std::move(o.m_mutability) }
			{}

		//! Swap operation.
		friend void
		swap(
			msg_type_and_handler_pair_t & a,
			msg_type_and_handler_pair_t & b ) SO_5_NOEXCEPT
			{
				std::swap( a.m_msg_type, b.m_msg_type );
				a.m_handler.swap( b.m_handler );
				std::swap( a.m_mutability, b.m_mutability );
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
		operator=( msg_type_and_handler_pair_t && o ) SO_5_NOEXCEPT
			{
				msg_type_and_handler_pair_t tmp{ std::move(o) };
				swap( *this, tmp );
				return *this;
			}

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

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::event_handler_method_t
 * instead.
 */
using event_handler_method_t = so_5::event_handler_method_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::demand_handler_pfn_t
 * instead.
 */
using demand_handler_pfn_t = so_5::demand_handler_pfn_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::execution_demand_t
 * instead.
 */
using execution_demand_t = so_5::execution_demand_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::execution_hint_t
 * instead.
 */
using execution_hint_t = so_5::execution_hint_t;

} /* namespace rt */

} /* namespace so_5 */

