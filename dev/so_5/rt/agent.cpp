/*
	SObjectizer 5.
*/

#include <so_5/rt/h/agent.hpp>
#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/environment.hpp>

#include <so_5/rt/impl/h/internal_env_iface.hpp>

#include <so_5/rt/impl/h/state_listener_controller.hpp>
#include <so_5/rt/impl/h/subscription_storage_iface.hpp>
#include <so_5/rt/impl/h/process_unhandled_exception.hpp>
#include <so_5/rt/impl/h/message_limit_internals.hpp>
#include <so_5/rt/impl/h/delivery_filter_storage.hpp>
#include <so_5/rt/impl/h/msg_tracing_helpers.hpp>

#include <so_5/details/h/abort_on_fatal_error.hpp>

#include <so_5/h/spinlocks.hpp>

#include <algorithm>
#include <sstream>
#include <cstdlib>

namespace so_5
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
// state_t::time_limit_t
//
struct state_t::time_limit_t
{
	struct timeout : public signal_t {};

	duration_t m_limit;
	const state_t & m_state_to_switch;

	mbox_t m_unique_mbox;
	timer_id_t m_timer;

	time_limit_t(
		duration_t limit,
		const state_t & state_to_switch )
		:	m_limit{ limit }
		,	m_state_to_switch{ state_to_switch }
	{}

	void
	set_up_limit_for_agent(
		agent_t & agent,
		const state_t & current_state ) SO_5_NOEXCEPT
	{
		// Because this method is called from on_enter handler it can't
		// throw exceptions. Any exception will lead to abort of the application.
		// So we don't care about exception safety.
		so_5::details::invoke_noexcept_code( [&] {

			// New unique mbox is necessary for time limit.
			m_unique_mbox = impl::internal_env_iface_t{ agent.so_environment() }
					// A new MPSC mbox will be used for that.
					.create_mpsc_mbox(
							// New MPSC mbox will be directly connected to target agent.
							&agent,
							// Message limits will not be used.
							nullptr );

			// A subscription must be created for timeout signal.
			agent.so_subscribe( m_unique_mbox )
					.in( current_state )
					.event< timeout >( [&agent, this] {
						agent.so_change_state( m_state_to_switch );
					} );

			// Delayed timeout signal must be sent.
			m_timer = agent.so_environment().schedule_timer< timeout >(
					m_unique_mbox,
					m_limit,
					duration_t::zero() );
		} );
	}

	void
	drop_limit_for_agent(
		agent_t & agent,
		const state_t & current_state ) SO_5_NOEXCEPT
	{
		// Because this method is called from on_exit handler it can't
		// throw exceptions. Any exception will lead to abort of the application.
		// So we don't care about exception safety.
		so_5::details::invoke_noexcept_code( [&] {
			m_timer.release();

			if( m_unique_mbox )
			{
				// Old subscription must be removed.
				agent.so_drop_subscription< timeout >( m_unique_mbox, current_state );
				// Unique mbox is no more needed.
				m_unique_mbox = mbox_t{};
			}
		} );
	}
};

//
// state_t
//

state_t::state_t(
	agent_t * target_agent,
	std::string state_name,
	state_t * parent_state,
	std::size_t nested_level,
	history_t state_history )
	:	m_target_agent{ target_agent }
	,	m_state_name( std::move(state_name) )
	,	m_parent_state{ parent_state }
	,	m_initial_substate{ nullptr }
	,	m_state_history{ state_history }
	,	m_last_active_substate{ nullptr }
	,	m_nested_level{ nested_level }
	,	m_substate_count{ 0 }
{
	if( parent_state )
	{
		// We should check the deep of nested states.
		if( m_nested_level >= max_deep )
			SO_5_THROW_EXCEPTION( rc_state_nesting_is_too_deep,
					"max nesting deep for agent states is " +
					std::to_string( max_deep ) );

		// Now we can safely mark parent state as composite.
		parent_state->m_substate_count += 1;
	}
}

state_t::state_t(
	agent_t * agent )
	:	state_t{ agent, history_t::none }
{
}

state_t::state_t(
	agent_t * agent,
	history_t state_history )
	:	state_t{ agent, std::string(), nullptr, 0, state_history }
{
}

state_t::state_t(
	agent_t * agent,
	std::string state_name )
	:	state_t{ agent, std::move(state_name), history_t::none }
{}

state_t::state_t(
	agent_t * agent,
	std::string state_name,
	history_t state_history )
	:	state_t{ agent, std::move(state_name), nullptr, 0, state_history }
{}

state_t::state_t(
	initial_substate_of parent )
	:	state_t{ parent, std::string(), history_t::none }
{} 

state_t::state_t(
	initial_substate_of parent,
	std::string state_name )
	:	state_t{ parent, std::move(state_name), history_t::none }
{}

state_t::state_t(
	initial_substate_of parent,
	std::string state_name,
	history_t state_history )
	:	state_t{
			parent.m_parent_state->m_target_agent,
			std::move(state_name),
			parent.m_parent_state,
			parent.m_parent_state->m_nested_level + 1,
			state_history }
{
	if( m_parent_state->m_initial_substate )
		SO_5_THROW_EXCEPTION( rc_initial_substate_already_defined,
				"initial substate for state " + m_parent_state->query_name() +
				" is already defined: " +
				m_parent_state->m_initial_substate->query_name() );

	m_parent_state->m_initial_substate = this;
}

state_t::state_t(
	substate_of parent )
	:	state_t{ parent, std::string(), history_t::none }
{}

state_t::state_t(
	substate_of parent,
	std::string state_name )
	:	state_t{ parent, std::move(state_name), history_t::none }
{}

state_t::state_t(
	substate_of parent,
	std::string state_name,
	history_t state_history )
	:	state_t{
			parent.m_parent_state->m_target_agent,
			std::move(state_name),
			parent.m_parent_state,
			parent.m_parent_state->m_nested_level + 1,
			state_history }
{}

state_t::state_t(
	state_t && other )
	:	m_target_agent( other.m_target_agent )
	,	m_state_name( std::move( other.m_state_name ) )
	,	m_parent_state{ other.m_parent_state }
	,	m_initial_substate{ other.m_initial_substate }
	,	m_state_history{ other.m_state_history }
	,	m_last_active_substate{ other.m_last_active_substate }
	,	m_nested_level{ other.m_nested_level }
	,	m_substate_count{ other.m_substate_count }
	,	m_on_enter{ std::move(other.m_on_enter) }
	,	m_on_exit{ std::move(other.m_on_exit) }
{
	if( m_parent_state && m_parent_state->m_initial_substate == &other )
		m_parent_state->m_initial_substate = this;
}

state_t::~state_t()
{
}

bool
state_t::operator == ( const state_t & state ) const
{
	return &state == this;
}

std::string
state_t::query_name() const
{
	auto getter = [this]() -> std::string {
		if( m_state_name.empty() )
			return create_anonymous_state_name( m_target_agent, this );
		else
			return m_state_name;
	};

	if( m_parent_state )
		return m_parent_state->query_name() + "." + getter();
	else
		return getter();
}

namespace {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif

/*!
 * \since v.5.4.0
 * \brief A special object for the state in which agent is awaiting
 * for deregistration after unhandled exception.
 *
 * This object will be shared between all agents.
 */
const state_t awaiting_deregistration_state(
		nullptr, "<AWAITING_DEREGISTRATION_AFTER_UNHANDLED_EXCEPTION>" );

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

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

state_t &
state_t::time_limit(
	duration_t timeout,
	const state_t & state_to_switch )
{
	if( duration_t::zero() == timeout )
		SO_5_THROW_EXCEPTION( rc_invalid_time_limit_for_state,
				"zero can't be used as time limit for state '" +
				query_name() );

	// Old time limit must be dropped if it exists.
	drop_time_limit();
	m_time_limit.reset( new time_limit_t{ timeout, state_to_switch } );

	// If this state is active then new time limit must be activated.
	if( is_active() )
		so_5::details::do_with_rollback_on_exception(
			[&] {
				m_time_limit->set_up_limit_for_agent( *m_target_agent, *this );
			},
			[&] {
				// Time limit must be dropped because it is not activated
				// for the current state.
				drop_time_limit();
			} );

	return *this;
}

state_t &
state_t::drop_time_limit()
{
	if( m_time_limit )
	{
		m_time_limit->drop_limit_for_agent( *m_target_agent, *this );
		m_time_limit.reset();
	}

	return *this;
}

const state_t *
state_t::actual_state_to_enter() const
{
	const state_t * s = this;
	while( 0 != s->m_substate_count )
	{
		if( s->m_last_active_substate )
			// Note: for states with shallow history m_last_active_substate
			// can point to composite substate. This substate must be
			// processed usual way with checking for substate count, presence
			// of initial substate and so on...
			s = s->m_last_active_substate;
		else if( !s->m_initial_substate )
			SO_5_THROW_EXCEPTION( rc_no_initial_substate,
					"there is no initial substate for composite state: " +
					query_name() );
		else
			s = s->m_initial_substate;
	}

	return s;
}

void
state_t::update_history_in_parent_states() const
{
	auto p = m_parent_state;

	// This pointer will be used for update states with shallow history.
	// This pointer will be changed on every iteration.
	auto c = this;

	while( p )
	{
		if( history_t::shallow == p->m_state_history )
			p->m_last_active_substate = c;
		else if( history_t::deep == p->m_state_history )
			p->m_last_active_substate = this;

		c = p;
		p = p->m_parent_state;
	}
}

void
state_t::handle_time_limit_on_enter() const
{
	m_time_limit->set_up_limit_for_agent( *m_target_agent, *this );
}

void
state_t::handle_time_limit_on_exit() const
{
	m_time_limit->drop_limit_for_agent( *m_target_agent, *this );
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
	:	agent_t( context_t{ env, std::move( options ) } )
{
}

agent_t::agent_t(
	context_t ctx )
	:	m_current_state_ptr( &st_default )
	,	m_was_defined( false )
	,	m_state_listener_controller( new impl::state_listener_controller_t )
	,	m_handler_finder{
			// Actual handler finder is dependent on msg_tracing status.
			impl::internal_env_iface_t{ ctx.env() }.is_msg_tracing_enabled() ?
				&agent_t::handler_finder_msg_tracing_enabled :
				&agent_t::handler_finder_msg_tracing_disabled }
	,	m_subscriptions(
			ctx.options().query_subscription_storage_factory()( self_ptr() ) )
	,	m_message_limits(
			message_limit::impl::info_storage_t::create_if_necessary(
				ctx.options().giveout_message_limits() ) )
	,	m_env( ctx.env() )
	,	m_event_queue( nullptr )
	,	m_direct_mbox(
			impl::internal_env_iface_t{ ctx.env() }.create_mpsc_mbox(
				self_ptr(),
				m_message_limits.get() ) )
		// It is necessary to enable agent subscription in the
		// constructor of derived class.
	,	m_working_thread_id( so_5::query_current_thread_id() )
	,	m_agent_coop( 0 )
	,	m_priority( ctx.options().query_priority() )
{
}

agent_t::~agent_t()
{
	// Sometimes it is possible that agent is destroyed without
	// correct deregistration from SO Environment.
	drop_all_delivery_filters();
	m_subscriptions.reset();
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

bool
agent_t::so_is_active_state( const state_t & state_to_check ) const
{
	state_t::path_t path;
	m_current_state_ptr->fill_path( path );

	auto e = begin(path) + m_current_state_ptr->nested_level() + 1;

	return e != std::find( begin(path), e, &state_to_check );
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
		auto actual_new_state = new_state.actual_state_to_enter();
		if( !( *actual_new_state == *m_current_state_ptr ) )
		{
			// New state differs from the current one.
			// Actual state switch must be performed.
			do_state_switch( *actual_new_state );

			// State listener should be informed.
			m_state_listener_controller->changed(
				*this,
				*m_current_state_ptr );
		}
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
agent_t::so_environment() const
{
	return m_env;
}

void
agent_t::so_bind_to_dispatcher(
	event_queue_t & queue )
{
	std::lock_guard< default_rw_spinlock_t > queue_lock{ m_event_queue_lock };

	// Cooperation usage counter should be incremented.
	// It will be decremented during final agent event execution.
	coop_t::increment_usage_count( *m_agent_coop );

	so_5::details::invoke_noexcept_code( [&] {
			// A starting demand must be sent first.
			queue.push(
					execution_demand_t(
							this,
							message_limit::control_block_t::none(),
							0,
							typeid(void),
							message_ref_t(),
							&agent_t::demand_handler_on_start ) );
			
			// Only then pointer to the queue could be stored.
			m_event_queue = &queue;
		} );
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
			auto handler = d.m_receiver->m_handler_finder(
					d, "create_execution_hint" );
			if( is_message_demand )
				{
					if( handler )
						return execution_hint_t(
								d,
								[handler](
										execution_demand_t & demand,
										current_thread_id_t thread_id ) {
									process_message(
											thread_id,
											demand,
											handler->m_method );
								},
								handler->m_thread_safety );
					else
						// Handler not found.
						return execution_hint_t::create_empty_execution_hint( d );
				}
			else
				// There must be a special hint for service requests
				// because absence of service handler processed by
				// different way than absence of event handler.
				return execution_hint_t(
						d,
						[handler](
								execution_demand_t & demand,
								current_thread_id_t thread_id ) {
							process_service_request(
									thread_id,
									demand,
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
				d,
				[]( execution_demand_t & demand,
					current_thread_id_t thread_id ) {
					demand.call_handler( thread_id );
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
	so_deregister_agent_coop( dereg_reason::normal );
}

agent_ref_t
agent_t::create_ref()
{
	agent_ref_t agent_ref( this );
	return agent_ref;
}

void
agent_t::bind_to_coop( coop_t & coop )
{
	m_agent_coop = &coop;
}

void
agent_t::shutdown_agent() SO_5_NOEXCEPT
{
	std::lock_guard< default_rw_spinlock_t > queue_lock{ m_event_queue_lock };

	// Since v.5.5.8 shutdown is done by two simple step:
	// - remove actual value from m_event_queue;
	// - pushing final demand to actual event queue.
	// 
	// No new demands will be sent to the agent, but all the subscriptions
	// remains. They will be destroyed at the very end of agent's lifetime.

	if( m_event_queue )
	{
		// Final event must be pushed to queue.
		so_5::details::invoke_noexcept_code( [&] {
				m_event_queue->push(
						execution_demand_t(
								this,
								message_limit::control_block_t::none(),
								0,
								typeid(void),
								message_ref_t(),
								&agent_t::demand_handler_on_finish ) );
			} );

		// No more events will be stored to the queue.
		m_event_queue = nullptr;
	}
	else
		so_5::details::abort_on_fatal_error( [&] {
			SO_5_LOG_ERROR( so_environment(), log_stream )
			{
				log_stream << "Unexpected error: m_event_queue contains "
					"nullptr. Unable to push demand_handler_on_finish for "
					"the agent (" << this << "). Application will be aborted"
					<< std::endl;
			}
		} );
}

void
agent_t::create_event_subscription(
	const mbox_t & mbox_ref,
	std::type_index msg_type,
	const state_t & target_state,
	const event_handler_method_t & method,
	thread_safety_t thread_safety )
{
	// Since v.5.4.0 there is no need for locking agent's mutex
	// because this operation can be performed only on agent's
	// working thread.

	ensure_operation_is_on_working_thread( "create_event_subscription" );

	m_subscriptions->create_event_subscription(
			mbox_ref,
			msg_type,
			detect_limit_for_message_type( msg_type ),
			target_state,
			method,
			thread_safety );
}

const message_limit::control_block_t *
agent_t::detect_limit_for_message_type(
	const std::type_index & msg_type ) const
{
	const message_limit::control_block_t * result = nullptr;

	if( m_message_limits )
	{
		result = m_message_limits->find( msg_type );
		if( !result )
			SO_5_THROW_EXCEPTION(
					so_5::rc_message_has_no_limit_defined,
					std::string( "an attempt to subscribe to message type without "
					"predefined limit for that type, type: " ) +
					msg_type.name() );
	}

	return result;
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
	const message_limit::control_block_t * limit,
	mbox_id_t mbox_id,
	std::type_index msg_type,
	const message_ref_t & message )
{
	read_lock_guard_t< default_rw_spinlock_t > queue_lock{ m_event_queue_lock };

	if( m_event_queue )
		m_event_queue->push(
				execution_demand_t(
					this,
					limit,
					mbox_id,
					msg_type,
					message,
					&agent_t::demand_handler_on_message ) );
}

void
agent_t::push_service_request(
	const message_limit::control_block_t * limit,
	mbox_id_t mbox_id,
	std::type_index msg_type,
	const message_ref_t & message )
{
	read_lock_guard_t< default_rw_spinlock_t > queue_lock{ m_event_queue_lock };

	if( m_event_queue )
		m_event_queue->push(
				execution_demand_t(
						this,
						limit,
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
	d.m_receiver->ensure_binding_finished();

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

void
agent_t::ensure_binding_finished()
{
	// Nothing more to do.
	// Just lock coop's binding_lock. If cooperation is not finished yet
	// it would stop the current thread.
	std::lock_guard< std::mutex > binding_lock{ m_agent_coop->m_binding_lock };
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

		// Since v.5.5.15 agent should be returned in default state.
		d.m_receiver->return_to_default_state_if_possible();
	}

	// Cooperation should receive notification about agent deregistration.
	coop_t::decrement_usage_count( *(d.m_receiver->m_agent_coop) );
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
	message_limit::control_block_t::decrement( d.m_limit );

	auto handler = d.m_receiver->m_handler_finder(
			d, "demand_handler_on_message" );
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
	message_limit::control_block_t::decrement( d.m_limit );

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
	msg_service_request_base_t::dispatch_wrapper(
		d.m_message_ref,
		[&] {
			const impl::event_handler_data_t * handler =
					handler_data.first ?
							handler_data.second :
							d.m_receiver->m_handler_finder(
									d, "process_service_request" );
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
		} );
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

void
agent_t::drop_all_delivery_filters() SO_5_NOEXCEPT
{
	if( m_delivery_filters )
	{
		m_delivery_filters->drop_all( *this );
		m_delivery_filters.reset();
	}
}

void
agent_t::do_set_delivery_filter(
	const mbox_t & mbox,
	const std::type_index & msg_type,
	delivery_filter_unique_ptr_t filter )
{
	ensure_operation_is_on_working_thread( "set_delivery_filter" );

	if( !m_delivery_filters )
		m_delivery_filters.reset( new impl::delivery_filter_storage_t() );

	m_delivery_filters->set_delivery_filter(
			mbox,
			msg_type,
			std::move(filter),
			*this );
}

void
agent_t::do_drop_delivery_filter(
	const mbox_t & mbox,
	const std::type_index & msg_type ) SO_5_NOEXCEPT
{
	ensure_operation_is_on_working_thread( "set_delivery_filter" );

	if( m_delivery_filters )
		m_delivery_filters->drop_delivery_filter( mbox, msg_type, *this );
}

const impl::event_handler_data_t *
agent_t::handler_finder_msg_tracing_disabled(
	execution_demand_t & d,
	const char * /*context_marker*/ )
{
	return find_event_handler_for_current_state( d );
}

const impl::event_handler_data_t *
agent_t::handler_finder_msg_tracing_enabled(
	execution_demand_t & d,
	const char * context_marker )
{
	const auto search_result = find_event_handler_for_current_state( d );

	impl::msg_tracing_helpers::trace_event_handler_search_result(
			d,
			context_marker,
			search_result );

	return search_result;
}

const impl::event_handler_data_t *
agent_t::find_event_handler_for_current_state(
	execution_demand_t & d )
{
	const impl::event_handler_data_t * search_result = nullptr;
	const state_t * s = &d.m_receiver->so_current_state();

	do {
		search_result = d.m_receiver->m_subscriptions->find_handler(
				d.m_mbox_id,
				d.m_msg_type, 
				*s );

		if( !search_result )
			s = s->parent_state();

	} while( search_result == nullptr && s != nullptr );

	return search_result;
}

void
agent_t::do_state_switch(
	const state_t & state_to_be_set )
{
	state_t::path_t old_path;
	state_t::path_t new_path;

	m_current_state_ptr->fill_path( old_path );
	state_to_be_set.fill_path( new_path );

	// Find the first item which is different in the paths.
	std::size_t first_diff = 0;
	for(; first_diff < std::min(
				m_current_state_ptr->nested_level(),
				state_to_be_set.nested_level() );
			++first_diff )
		if( old_path[ first_diff ] != new_path[ first_diff ] )
			break;

	// Do call for on_exit and on_enter for states.
	// on_exit and on_enter should not throw exceptions.
	so_5::details::invoke_noexcept_code( [&] {

		impl::msg_tracing_helpers::safe_trace_state_leaving(
				*this, *m_current_state_ptr );
		for( std::size_t i = m_current_state_ptr->nested_level();
				i >= first_diff; )
			{
				old_path[ i ]->call_on_exit();
				if( i )
					--i;
				else
					break;
			}

		impl::msg_tracing_helpers::safe_trace_state_entering(
				*this, state_to_be_set );
		for( std::size_t i = first_diff;
				i <= state_to_be_set.nested_level();
				++i )
			{
				new_path[ i ]->call_on_enter();
			}
	} );

	// Now the current state for the agent can be changed.
	m_current_state_ptr = &state_to_be_set;
	m_current_state_ptr->update_history_in_parent_states();
}

void
agent_t::return_to_default_state_if_possible() SO_5_NOEXCEPT
{
	if( !( st_default == so_current_state() ||
			awaiting_deregistration_state == so_current_state() ) )
	{
		// The agent must be returned to the default state.
		// All on_exit handlers must be called at this point.
		so_change_state( st_default );
	}
}

} /* namespace so_5 */

