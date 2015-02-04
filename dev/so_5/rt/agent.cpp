/*
	SObjectizer 5.
*/

#include <so_5/rt/h/agent.hpp>
#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/environment.hpp>

#include <so_5/rt/impl/h/state_listener_controller.hpp>
#include <so_5/rt/impl/h/subscription_storage_iface.hpp>
#include <so_5/rt/impl/h/process_unhandled_exception.hpp>

#include <sstream>
#include <cstdlib>

namespace so_5
{

namespace rt
{

namespace
{

/*!
 * \since v.5.4.0
 * \brief A helper class for temporary setting and then dropping
 * the ID of the current working thread.
 *
 * \note New working thread_id is set only if it is not an
 * null thread_id.
 */
struct working_thread_id_sentinel_t
	{
		so_5::current_thread_id_t & m_id;

		working_thread_id_sentinel_t(
			so_5::current_thread_id_t & id_var,
			so_5::current_thread_id_t value_to_set )
			:	m_id( id_var )
			{
				if( value_to_set != null_current_thread_id() )
					m_id = value_to_set;
			}
		~working_thread_id_sentinel_t()
			{
				if( m_id != null_current_thread_id() )
					m_id = null_current_thread_id();
			}
	};

/*!
 * \since v.5.4.0
 */
std::string
create_anonymous_state_name( const agent_t * agent, const state_t * st )
	{
		std::ostringstream ss;
		ss << "<state:target=" << agent << ":this=" << st << ">";
		return ss.str();
	}

} /* namespace anonymous */

// NOTE: Implementation of state_t is moved to that file in v.5.4.0.

//
// state_t
//

state_t::state_t(
	agent_t * agent )
	:	m_target_agent( agent )
	,	m_state_name( create_anonymous_state_name( agent, self_ptr() ) )
{
}

state_t::state_t(
	agent_t * agent,
	std::string state_name )
	:
		m_target_agent( agent ),
		m_state_name( state_name.empty() ?
				create_anonymous_state_name( agent, self_ptr() ) :
				std::move(state_name) )
{
}

state_t::state_t(
	state_t && other )
	:	m_target_agent( other.m_target_agent )
	,	m_state_name( std::move( other.m_state_name ) )
{}

state_t::~state_t()
{
}

bool
state_t::operator == ( const state_t & state ) const
{
	return &state == this;
}

const std::string &
state_t::query_name() const
{
	return m_state_name;
}

namespace {

/*!
 * \since v.5.4.0
 * \brief A special object for the state in which agent is awaiting
 * for deregistration after unhandled exception.
 *
 * This object will be shared between all agents.
 */
const state_t awaiting_deregistration_state(
		nullptr, "<AWAITING_DEREGISTRATION_AFTER_UNHANDLED_EXCEPTION>" );

} /* namespace anonymous */

bool
state_t::is_target( const agent_t * agent ) const
{
	if( m_target_agent )
		return m_target_agent == agent;
	else if( this == &awaiting_deregistration_state )
		return true;
	else
		return false;
}

void
state_t::activate() const
{
	m_target_agent->so_change_state( *this );
}

//
// agent_t
//

agent_t::agent_t(
	environment_t & env )
	:	agent_t( env, tuning_options() )
{
}

agent_t::agent_t(
	environment_t & env,
	agent_tuning_options_t options )
	:	m_current_state_ptr( &st_default )
	,	m_was_defined( false )
	,	m_state_listener_controller( new impl::state_listener_controller_t )
	,	m_subscriptions(
			options.query_subscription_storage_factory()( self_ptr() ) )
	,	m_env( env )
	,	m_event_queue_proxy( new event_queue_proxy_t() )
	,	m_tmp_event_queue( m_mutex )
	,	m_direct_mbox(
			env.so5__create_mpsc_mbox(
				self_ptr(),
				m_event_queue_proxy ) )
		// It is necessary to enable agent subscription in the
		// constructor of derived class.
	,	m_working_thread_id( so_5::query_current_thread_id() )
	,	m_agent_coop( 0 )
	,	m_is_coop_deregistered( false )
{
	m_event_queue_proxy->switch_to( m_tmp_event_queue );
}

agent_t::~agent_t()
{
	// Sometimes it is possible that agent is destroyed without
	// correct deregistration from SO Environment.
	m_subscriptions.reset();

	m_event_queue_proxy->shutdown();
}

void
agent_t::so_evt_start()
{
	// Default implementation do nothing.
}

void
agent_t::so_evt_finish()
{
	// Default implementation do nothing.
}


const std::string &
agent_t::so_coop_name() const
{
	if( 0 == m_agent_coop )
		throw exception_t(
			"agent isn't bound to cooperation yet",
			rc_agent_has_no_cooperation );

	return m_agent_coop->query_coop_name();
}

void
agent_t::so_add_nondestroyable_listener(
	agent_state_listener_t & state_listener )
{
	m_state_listener_controller->so_add_nondestroyable_listener(
		state_listener );
}

void
agent_t::so_add_destroyable_listener(
	agent_state_listener_unique_ptr_t state_listener )
{
	m_state_listener_controller->so_add_destroyable_listener(
		std::move( state_listener ) );
}

exception_reaction_t
agent_t::so_exception_reaction() const
{
	if( m_agent_coop )
		return m_agent_coop->exception_reaction();
	else
		// This is very strange case. So it would be better to abort.
		return abort_on_exception;
}

void
agent_t::so_switch_to_awaiting_deregistration_state()
{
	so_change_state( awaiting_deregistration_state );
}

const mbox_t &
agent_t::so_direct_mbox() const
{
	return m_direct_mbox;
}

const state_t &
agent_t::so_default_state() const
{
	return st_default;
}

void
agent_t::so_change_state(
	const state_t & new_state )
{
	ensure_operation_is_on_working_thread( "so_change_state" );

	if( new_state.is_target( this ) )
	{
		m_current_state_ptr = &new_state;

		// State listener should be informed.
		m_state_listener_controller->changed(
			*this,
			*m_current_state_ptr );
	}
	else
		SO_5_THROW_EXCEPTION(
			rc_agent_unknown_state,
			"unable to switch agent to alien state "
			"(the state that doesn't belong to this agent)" );
}

void
agent_t::so_initiate_agent_definition()
{
	working_thread_id_sentinel_t sentinel(
			m_working_thread_id,
			so_5::query_current_thread_id() );

	so_define_agent();

	m_was_defined = true;
}

void
agent_t::so_define_agent()
{
	// Default implementation do nothing.
}

bool
agent_t::so_was_defined() const
{
	return m_was_defined;
}

environment_t &
agent_t::so_environment()
{
	return m_env;
}

void
agent_t::so_bind_to_dispatcher(
	event_queue_t & queue )
{
	// Cooperation usage counter should be incremented.
	// It will be decremented during final agent event execution.
	agent_coop_t::increment_usage_count( *m_agent_coop );

	m_tmp_event_queue.switch_to_actual_queue(
			queue,
			this,
			&agent_t::demand_handler_on_start );

	// Proxy must be switched on unblocked agent.
	// Otherwise there could be a deadlock when direct mbox is used.
	// Scenario:
	//
	// T1:
	//  - is trying to send message to the agent;
	//  - event_queue_proxy spinlock is locked in 'reader' mode;
	//  - tmp_queue.push is called;
	//  - tmp_queue.push is trying to acquire agent's mutex;
	// T2:
	//  - is trying to bind agent to the dispatcher;
	//  - tmp_queue.switch_to_actual_queue is called;
	//  - agent's mutex is acquired;
	//  - an attempt to switch proxy to actual queue is performed;
	//  - is trying to acquire proxy's spinlock if 'writer' mode.
	//
	// Becuase of that m_event_queue_proxy->switch_to is now called
	// outside of m_tmp_event_queue.switch_to_actual_queue().
	//
	m_event_queue_proxy->switch_to( queue );
}

execution_hint_t
agent_t::so_create_execution_hint(
	execution_demand_t & d )
{
	static const demand_handler_pfn_t message_handler =
			&agent_t::demand_handler_on_message;

	const bool is_message_demand = (message_handler == d.m_demand_handler);
	const bool is_service_demand = !is_message_demand &&
			(&agent_t::service_request_handler_on_message == d.m_demand_handler);

	if( is_message_demand || is_service_demand )
		{
			// Try to find handler for the demand.
			auto handler = d.m_receiver->m_subscriptions->find_handler(
					d.m_mbox_id,
					d.m_msg_type, 
					d.m_receiver->so_current_state() );
			if( is_message_demand )
				{
					if( handler )
						return execution_hint_t(
								[&d, handler]( current_thread_id_t thread_id ) {
									process_message(
											thread_id,
											d,
											handler->m_method );
								},
								handler->m_thread_safety );
					else
						// Handler not found.
						return execution_hint_t::create_empty_execution_hint();
				}
			else
				// There must be a special hint for service requests
				// because absence of service handler processed by
				// different way than absence of event handler.
				return execution_hint_t(
						[&d, handler]( current_thread_id_t thread_id ) {
							process_service_request(
									thread_id,
									d,
									std::make_pair( true, handler ) );
						},
						handler ? handler->m_thread_safety :
							// If there is no real handler then
							// there will be only error processing.
							// That processing is thread safe.
							thread_safe );
		}
	else
		// This is demand_handler_on_start or demand_handler_on_finish.
		return execution_hint_t(
				[&d]( current_thread_id_t thread_id ) {
						d.m_demand_handler( thread_id, d );
				},
				not_thread_safe );
}

void
agent_t::so_deregister_agent_coop( int dereg_reason )
{
	so_environment().deregister_coop(
			so_coop_name(), dereg_reason );
}

void
agent_t::so_deregister_agent_coop_normally()
{
	so_deregister_agent_coop( so_5::rt::dereg_reason::normal );
}

agent_ref_t
agent_t::create_ref()
{
	agent_ref_t agent_ref( this );
	return agent_ref;
}

void
agent_t::bind_to_coop(
	agent_coop_t & coop )
{
	m_agent_coop = &coop;
	m_is_coop_deregistered = false;
}

void
agent_t::shutdown_agent()
{
	// Since v.5.4.0.1 shutdown is done by one simple step: shutdown
	// of event_queue_proxy objects. No new demands will be sent to
	// the agent, but all the subscriptions remains. They will be destroyed
	// at the very end of agent's lifetime.

	// e must shutdown proxy object. And only then
	// the last demand will be sent to the agent.
	auto q = m_event_queue_proxy->shutdown();
	if( q )
		q->push(
				execution_demand_t(
						this,
						0,
						typeid(void),
						message_ref_t(),
						&agent_t::demand_handler_on_finish ) );
	else
	{
		SO_5_LOG_ERROR( so_environment(), log_stream )
		{
			log_stream << "Unexpected error: m_event_queue_proxy->shutdown() "
				"returns nullptr. Unable to push demand_handler_on_finish for "
				"the agent (" << this << "). Application will be aborted"
				<< std::endl;
			std::abort();
		}
	}
}

namespace
{
	template< class S >
	bool is_known_mbox_msg_pair(
		S & s,
		typename S::iterator it )
	{
		if( it != s.begin() )
		{
			typename S::iterator prev = it;
			++prev;
			if( it->first.is_same_mbox_msg_pair( prev->first ) )
				return true;
		}

		typename S::iterator next = it;
		++next;
		if( next != s.end() )
			return it->first.is_same_mbox_msg_pair( next->first );

		return false;
	}

} /* namespace anonymous */

void
agent_t::create_event_subscription(
	const mbox_t & mbox_ref,
	std::type_index type_index,
	const state_t & target_state,
	const event_handler_method_t & method,
	thread_safety_t thread_safety )
{
	// Since v.5.4.0 there is no need for locking agent's mutex
	// because this operation can be performed only on agent's
	// working thread.

	ensure_operation_is_on_working_thread( "create_event_subscription" );

	if( m_is_coop_deregistered )
		return;

	m_subscriptions->create_event_subscription(
			mbox_ref, type_index, target_state, method, thread_safety );
}

void
agent_t::do_drop_subscription(
	const mbox_t & mbox,
	const std::type_index & msg_type,
	const state_t & target_state )
{
	ensure_operation_is_on_working_thread( "do_drop_subscription" );

	// Since v.5.4.0 there is no need for locking agent's mutex
	// because this operation can be performed only on agent's
	// working thread.

	m_subscriptions->drop_subscription( mbox, msg_type, target_state );
}

void
agent_t::do_drop_subscription_for_all_states(
	const mbox_t & mbox,
	const std::type_index & msg_type )
{
	// Since v.5.4.0 there is no need for locking agent's mutex
	// because this operation can be performed only on agent's
	// working thread.

	ensure_operation_is_on_working_thread(
			"do_drop_subscription_for_all_states" );

	m_subscriptions->drop_subscription_for_all_states( mbox, msg_type );
}

void
agent_t::push_event(
	mbox_id_t mbox_id,
	std::type_index msg_type,
	const message_ref_t & message )
{
	m_event_queue_proxy->push(
			execution_demand_t(
				this,
				mbox_id,
				msg_type,
				message,
				&agent_t::demand_handler_on_message ) );
}

void
agent_t::push_service_request(
	mbox_id_t mbox_id,
	std::type_index msg_type,
	const message_ref_t & message )
{
	m_event_queue_proxy->push(
			execution_demand_t(
					this,
					mbox_id,
					msg_type,
					message,
					&agent_t::service_request_handler_on_message ) );
}

void
agent_t::demand_handler_on_start(
	current_thread_id_t working_thread_id,
	execution_demand_t & d )
{
	working_thread_id_sentinel_t sentinel(
			d.m_receiver->m_working_thread_id,
			working_thread_id );

	try
	{
		d.m_receiver->so_evt_start();
	}
	catch( const std::exception & x )
	{
		impl::process_unhandled_exception(
				working_thread_id, x, *(d.m_receiver) );
	}
}

demand_handler_pfn_t
agent_t::get_demand_handler_on_start_ptr()
{
	return &agent_t::demand_handler_on_start;
}

void
agent_t::demand_handler_on_finish(
	current_thread_id_t working_thread_id,
	execution_demand_t & d )
{
	{
		// Sentinel must finish its work before decrementing
		// reference count to cooperation.
		working_thread_id_sentinel_t sentinel(
				d.m_receiver->m_working_thread_id,
				working_thread_id );

		try
		{
			d.m_receiver->so_evt_finish();
		}
		catch( const std::exception & x )
		{
			impl::process_unhandled_exception(
					working_thread_id, x, *(d.m_receiver) );
		}
	}

	// Cooperation should receive notification about agent deregistration.
	agent_coop_t::decrement_usage_count( *(d.m_receiver->m_agent_coop) );
}

demand_handler_pfn_t
agent_t::get_demand_handler_on_finish_ptr()
{
	return &agent_t::demand_handler_on_finish;
}

void
agent_t::demand_handler_on_message(
	current_thread_id_t working_thread_id,
	execution_demand_t & d )
{
	auto handler = d.m_receiver->m_subscriptions->find_handler(
			d.m_mbox_id,
			d.m_msg_type, 
			d.m_receiver->so_current_state() );
	if( handler )
		process_message( working_thread_id, d, handler->m_method );
}

demand_handler_pfn_t
agent_t::get_demand_handler_on_message_ptr()
{
	return &agent_t::demand_handler_on_message;
}

void
agent_t::service_request_handler_on_message(
	current_thread_id_t working_thread_id,
	execution_demand_t & d )
{
	static const impl::event_handler_data_t * const null_handler_data = nullptr;

	process_service_request(
			working_thread_id,
			d,
			std::make_pair( false, null_handler_data ) );
}

demand_handler_pfn_t
agent_t::get_service_request_handler_on_message_ptr()
{
	return &agent_t::service_request_handler_on_message;
}

void
agent_t::process_message(
	current_thread_id_t working_thread_id,
	execution_demand_t & d,
	const event_handler_method_t & method )
{
	working_thread_id_sentinel_t sentinel(
			d.m_receiver->m_working_thread_id,
			working_thread_id );

	try
	{
		method( invocation_type_t::event, d.m_message_ref );
	}
	catch( const std::exception & x )
	{
		impl::process_unhandled_exception(
				working_thread_id, x, *(d.m_receiver) );
	}
}

void
agent_t::process_service_request(
	current_thread_id_t working_thread_id,
	execution_demand_t & d,
	std::pair< bool, const impl::event_handler_data_t * > handler_data )
{
	try
		{
			const impl::event_handler_data_t * handler =
					handler_data.first ?
							handler_data.second :
							d.m_receiver->m_subscriptions->find_handler(
									d.m_mbox_id,
									d.m_msg_type, 
									d.m_receiver->so_current_state() );
			if( handler )
			{
				working_thread_id_sentinel_t sentinel(
						d.m_receiver->m_working_thread_id,
						working_thread_id );

				handler->m_method(
						invocation_type_t::service_request, d.m_message_ref );
			}
			else
				SO_5_THROW_EXCEPTION(
						so_5::rc_svc_not_handled,
						"service request handler is not found for "
								"the current agent state" );
		}
	catch( ... )
		{
			auto & svc_request =
					*(dynamic_cast< msg_service_request_base_t * >(
							d.m_message_ref.get() ));
			svc_request.set_exception( std::current_exception() );
		}
}

void
agent_t::ensure_operation_is_on_working_thread(
	const char * operation_name ) const
{
	if( so_5::query_current_thread_id() != m_working_thread_id )
	{
		std::ostringstream s;

		s << operation_name
			<< ": operation is enabled only on agent's working thread; "
			<< "working_thread_id: " << m_working_thread_id
			<< ", current_thread_id: " << so_5::query_current_thread_id();

		SO_5_THROW_EXCEPTION(
				so_5::rc_operation_enabled_only_on_agent_working_thread,
				s.str() );
	}
}

} /* namespace rt */

} /* namespace so_5 */

