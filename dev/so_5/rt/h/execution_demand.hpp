/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0
	\brief Event-related stuff.
*/

#if !defined( _SO_5__RT__EXECUTION_DEMAND_HPP_ )
#define _SO_5__RT__EXECUTION_DEMAND_HPP_

#include <so_5/h/types.hpp>
#include <so_5/h/current_thread_id.hpp>

#include <so_5/rt/h/message.hpp>

namespace so_5
{

namespace rt
{

class agent_t;

//
// event_handler_method_t
//
/*!
 * \since v.5.3.0
 * \brief Type of event handler method.
 */
typedef std::function< void(invocation_type_t, message_ref_t &) >
		event_handler_method_t;

struct execution_demand_t;

//
// demand_handler_pfn_t
//
/*!
 * \since v.5.2.0
 * \brief Demand handler prototype.
 */
typedef void (*demand_handler_pfn_t)(
	current_thread_id_t,
	execution_demand_t & );

//
// execution_demand_t
//
/*!
 * \since v.5.4.0
 * \brief A description of event execution demand.
 */
struct execution_demand_t
{
	//! Receiver of demand.
	agent_t * m_receiver;
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
		,	m_mbox_id( 0 )
		,	m_msg_type( typeid(void) )
		,	m_demand_handler( nullptr )
		{}

	execution_demand_t(
		agent_t * receiver,
		mbox_id_t mbox_id,
		std::type_index msg_type,
		message_ref_t message_ref,
		demand_handler_pfn_t demand_handler )
		:	m_receiver( receiver )
		,	m_mbox_id( mbox_id )
		,	m_msg_type( msg_type )
		,	m_message_ref( std::move( message_ref ) )
		,	m_demand_handler( demand_handler )
		{}
};

//
// execution_hint_t
//
/*!
 * \since v.5.4.0
 * \brief A hint for a dispatcher for execution of event
 * for the concrete execution_demand.
 */
class execution_hint_t
{
public :
	//! Type of function for calling event handler directly.
	typedef std::function< void( current_thread_id_t ) > direct_func_t;

	//! Initializing constructor.
	execution_hint_t(
		direct_func_t direct_func,
		thread_safety_t thread_safety )
		:	m_direct_func( std::move( direct_func ) )
		,	m_thread_safety( thread_safety )
		{}

	//! Is event handler defined for the demand?
	operator bool() const
		{
			return static_cast< bool >(m_direct_func);
		}

	//! Call event handler directly.
	void
	exec( current_thread_id_t working_thread_id ) const
		{
			m_direct_func( is_thread_safe() ?
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
	static execution_hint_t
	create_empty_execution_hint()
		{
std::cout << "create_empty_execution_hint" << std::endl;
			return execution_hint_t();
		}

private :
	//! Function for call event handler directly.
	direct_func_t m_direct_func;

	//! Thread safety for event handler.
	thread_safety_t m_thread_safety;

	//! Default constructor.
	/*!
	 * Useful when event handler for the demand not found.
	 */
	execution_hint_t()
		:	m_direct_func()
		,	m_thread_safety( thread_safe )
		{}
};

} /* namespace rt */

} /* namespace so_5 */

#endif

