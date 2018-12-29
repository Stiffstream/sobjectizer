/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Testing related stuff.
 *
 * \since
 * v.5.5.24
 */

#include <so_5/experimental/testing/v1/all.hpp>

namespace so_5 {
	
namespace experimental {
	
namespace testing {
	
SO_5_INLINE_NS namespace v1 {

namespace details {

trigger_t::trigger_t(
	incident_status_t incident_status,
	const agent_t & target,
	std::type_index msg_type,
	mbox_id_t src_mbox_id )
	:	m_incident_status( incident_status )
	,	m_target_agent( target )
	,	m_target_id( target.so_direct_mbox()->id() )
	,	m_msg_type( std::move(msg_type) )
	,	m_src_mbox_id( src_mbox_id )
	{}

trigger_t::~trigger_t()
	{}

SO_5_NODISCARD
const agent_t &
trigger_t::target_agent() const SO_5_NOEXCEPT
	{
		return m_target_agent;
	}

void
trigger_t::set_completion( completion_function_t fn )
	{
		m_completion = std::move(fn);
	}

SO_5_NODISCARD
bool
trigger_t::check(
	const incident_status_t incident_status,
	const incident_info_t & info ) const SO_5_NOEXCEPT
	{
		return incident_status == m_incident_status
				&& info.m_agent->so_direct_mbox()->id() == m_target_id
				&& info.m_msg_type == m_msg_type
				&& info.m_src_mbox_id == m_src_mbox_id;
	}

SO_5_NODISCARD
bool
trigger_t::requires_completion() const SO_5_NOEXCEPT
	{
		return static_cast<bool>(m_completion);
	}

void
trigger_t::complete(
	const trigger_completion_context_t & context ) SO_5_NOEXCEPT
	{
		m_completion( context );
	}

/*!
 * \brief An actual implementation of step of testing scenario.
 *
 * \since
 * v.5.5.24
 */
class real_scenario_step_t final : public abstract_scenario_step_t
	{
		//! Type of container for preactivation actions.
		using preactivate_actions_container_t =
				std::vector< preactivate_action_t >;

		//! Name of a step.
		const std::string m_name;

		//! All preactivation actions.
		preactivate_actions_container_t m_preactivate_actions;

		//! All constraints.
		constraint_container_t m_constraints;

		//! All triggers.
		/*!
		 * This containers holds all triggers as for when_all, as
		 * for when_any case.
		 *
		 * There is an important trick: activated triggers moved to
		 * the end of that container. It means that first N triggers
		 * in that containers wait for activation. The value of N
		 * is defined by m_last_non_activated_trigger attribute.
		 */
		trigger_container_t m_triggers;
		//! Index of last trigger in the first part of trigger's container.
		/*!
		 * Receives actual value in setup_triggers() method.
		 * At the begining, if m_triggers.size() is not 0, it holds
		 * (m_triggers.size()-1). Then it is decremented after activation
		 * of yet another trigger.
		 */
		std::size_t m_last_non_activated_trigger = 0u;

		//! Count of triggers those should be activated for activation
		//! of the step.
		/*!
		 * Receives actual value in setup_triggers() method.
		 */
		std::size_t m_triggers_to_activate;
		//! Count of triggers those are activated.
		std::size_t m_triggers_activated = 0u;

		//! Count of activated triggers those are not completed yet.
		/*!
		 * Condition for completion of the step looks like:
		 * \code
		 * (m_triggers_to_activate == m_triggers_activated) && (0u == m_triggers_to_completion)
		 * \endcode
		 */
		std::size_t m_triggers_to_completion = 0u;

		//! The current state of the step.
		status_t m_status = status_t::passive;

	public :
		real_scenario_step_t(
			std::string name )
			:	m_name( std::move(name) )
			{}

		SO_5_NODISCARD
		const std::string &
		name() const SO_5_NOEXCEPT override
			{
				return m_name;
			}

		void
		preactivate() SO_5_NOEXCEPT override
			{
				change_status( status_t::preactivated );
			}

		SO_5_NODISCARD
		token_t
		pre_handler_hook(
			const incident_info_t & info ) SO_5_NOEXCEPT override
			{
				token_t result;

				if( status_t::preactivated == m_status )
					result = try_activate( incident_status_t::handled, info );

				return result;
			}

		void
		post_handler_hook(
			const scenario_in_progress_accessor_t & scenario_accessor,
			token_t token ) SO_5_NOEXCEPT override
			{
				if( token.valid() )
					{
						token.trigger().complete(
								trigger_completion_context_t{
										scenario_accessor,
										*this
								} );

						--m_triggers_to_completion;

						// If step activated and there is no triggers to
						// be completed then the step can be completed.
						// But there can be cases when m_triggers_to_completion
						// is zero, but the step is not activated yet
						// (for example when `when_all` is used).
						if( !m_triggers_to_completion &&
								status_t::active == m_status )
							change_status( status_t::completed );
					}
			}

		void
		no_handler_hook(
			const incident_info_t & info ) SO_5_NOEXCEPT override
			{
				if( status_t::preactivated == m_status )
					(void)try_activate( incident_status_t::ignored, info );
			}

		SO_5_NODISCARD
		status_t
		status() const SO_5_NOEXCEPT override
			{
				return m_status;
			}

		void
		add_preactivate_action(
			preactivate_action_t action ) override
			{
				m_preactivate_actions.emplace_back( std::move(action) );
			}

		void
		setup_triggers(
			trigger_container_t triggers,
			std::size_t triggers_to_activate ) SO_5_NOEXCEPT override
			{
				using std::swap;

				swap( m_triggers, triggers );
				m_triggers_to_activate = triggers_to_activate;

				m_last_non_activated_trigger = m_triggers.size();
				if( m_last_non_activated_trigger )
					--m_last_non_activated_trigger;
			}

		void
		setup_constraints(
			constraint_container_t constraints ) SO_5_NOEXCEPT override
			{
				using std::swap;

				swap( m_constraints, constraints );
			}

	private :
		//! Change the state of the step and perform all necessary
		//! transitional actions.
		/*!
		 * If a new state is status_t::preactivated then preactivation
		 * actions will be called and constraint_t::start() will be called
		 * for all constraints.
		 *
		 * If a new state is status_t::completed then constraint_t::finish()
		 * will be called for all constraints.
		 */
		void
		change_status( status_t status ) SO_5_NOEXCEPT
			{
				m_status = status;
				switch( status )
					{
					case status_t::preactivated:
						{
							for( auto & act : m_preactivate_actions )
								act();

							for( auto & c : m_constraints )
								c->start();
						}
					break;
					case status_t::completed:
						{
							for( auto & c : m_constraints )
								c->finish();
						}
					break;

					case status_t::passive: break;
					case status_t::active: break;
					}
			}

		//! An attempt to check constraints for a new incident.
		/*!
		 * \retval true if all constraints fullfilled.
		 */
		SO_5_NODISCARD
		bool
		try_pass_constraints(
			const incident_status_t incident_status,
			const incident_info_t & info ) const SO_5_NOEXCEPT
			{
				for( auto & c : m_constraints )
					if( !c->check( incident_status, info ) )
						return false;

				return true;
			}

		//! An attempt to activate the step when a new incident arrives.
		SO_5_NODISCARD
		token_t
		try_activate(
			const incident_status_t incident_status,
			const incident_info_t & info ) SO_5_NOEXCEPT
			{
				token_t result;

				// All constraints must be fullfilled.
				if( !try_pass_constraints( incident_status, info ) )
					return result;

				// Check triggers those are not activated yet.
				// Those triggers are in the first part of m_triggers.
				// It is safe to add 1 to m_last_non_activated_trigger, because
				// this method is called only if there is at least one
				// non-activated trigger in the triggers container.
				auto end = std::begin(m_triggers) +
						static_cast<trigger_container_t::iterator::difference_type>(
								m_last_non_activated_trigger + 1u);

				auto it = std::find_if(
						std::begin(m_triggers), end,
						[incident_status, &info]( trigger_unique_ptr_t & trigger ) {
							return trigger->check( incident_status, info );
						} );

				if( it == end )
					return result;

				// Actual trigger should be stored separatelly.
				trigger_t * active_trigger = it->get();
				if( m_last_non_activated_trigger )
					{
						// Actual trigger should be moved to the end of triggers list.
						std::swap( *it, m_triggers[ m_last_non_activated_trigger ] );
						--m_last_non_activated_trigger;
					}

				++m_triggers_activated;
				if( active_trigger->requires_completion() )
					{
						++m_triggers_to_completion;
						// Actual trigger should be returned in result token.
						result = token_t{ active_trigger };
					}

				// Now the new state of the step can be calculated.
				if( m_triggers_activated == m_triggers_to_activate )
					{
						// All expected triggers are activated.
						// New state should be `active` or `completed`.
						change_status( (0u != m_triggers_to_completion) ?
								status_t::active : status_t::completed );
					}

				return result;
			}
	};

/*!
 * \brief An interface for object that will unfreeze all registered
 * agents when testing scenario starts.
 *
 * It is necessary for testing scenarios to keep all registered agents in
 * frozen state. Those agents should be present in the Environment but should
 * not receive nor handle messages. It is done by a special implementation of
 * event queues and event_queue_hook.
 *
 * But at some moment all frozen agents should become unfrozen. This moment is
 * known only for testing scenario. It means that testing scenario should
 * issue a command to unfreeze all frozen agents.
 *
 * This interface allows testing scenario to unfreeze agents but hides the
 * details of this procedure.
 *
 * \since
 * v.5.5.24
 */
class agent_unfreezer_t
	{
		agent_unfreezer_t( const agent_unfreezer_t & ) = delete;
		agent_unfreezer_t & operator=( const agent_unfreezer_t & ) = delete;

		agent_unfreezer_t( agent_unfreezer_t && ) = delete;
		agent_unfreezer_t & operator=( agent_unfreezer_t && ) = delete;

	public :
		agent_unfreezer_t() = default;
		virtual ~agent_unfreezer_t() = default;

		//! Issue a command to unfreeze all frozen agents.
		virtual void
		unfreeze() SO_5_NOEXCEPT = 0;
	};

/*!
 * \brief The actual implementation of testing scenario.
 *
 * \since
 * v.5.5.24
 */
class real_scenario_t final : public abstract_scenario_t
	{
	private :
		//! Object lock.
		/*!
		 * \note
		 * It is mutable because it has to be used in a const method.
		 */
		mutable std::mutex m_lock;
		//! Condition variable for waiting completion of the scenario.
		std::condition_variable m_completion_cv;

		//! The current state of the scenario.
		scenario_status_t m_status = scenario_status_t::not_started;

		//! Scenario's steps.
		/*!
		 * Can be empty.
		 */
		std::vector< step_unique_ptr_t > m_steps;
		//! Set of active step those are not completed yet.
		/*!
		 * If a step switches from preactivated state to the completed
		 * without staying in active state then this step won't go
		 * to this set.
		 */
		std::set< abstract_scenario_step_t * > m_active_steps;

		//! Index of the current preactivated step.
		std::size_t m_waiting_step_index = 0u;

		//! Type of container for holding stored state names.
		using state_name_map_t = std::map<
				std::pair< std::string, std::string >,
				std::string >;

		//! Container for holding stored state names.
		state_name_map_t m_stored_states;

		//! Unfreezer for registered agents.
		/*!
		 * \note Will receive an actual value later.
		 */
		agent_unfreezer_t * m_unfreezer{};

	public :
		real_scenario_t() = default;

		//! Set the unfreezer for registered agents.
		/*!
		 * \attention
		 * This method must be called before star of the scenario.
		 */
		void
		setup_unfreezer( agent_unfreezer_t & unfreezer ) SO_5_NOEXCEPT
			{
				m_unfreezer = &unfreezer;
			}

		SO_5_NODISCARD
		step_definition_proxy_t
		define_step( nonempty_name_t step_name ) override
			{
				std::lock_guard< std::mutex > lock{ m_lock };
				if( scenario_status_t::not_started != m_status )
					SO_5_THROW_EXCEPTION(
							rc_unable_to_define_new_step,
							"new testing scenario step can be defined only when "
							"scenario is not started yet" );

				m_steps.emplace_back(
						stdcpp::make_unique< real_scenario_step_t >(
								step_name.giveout_value() ) );

				return { m_steps.back().get() };
			}

		SO_5_NODISCARD
		scenario_result_t
		result() const SO_5_NOEXCEPT override
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				if( scenario_status_t::completed == m_status )
					return { scenario_status_t::completed };
				else
					return { m_status, describe_current_state() };
			}

		void
		run_for( std::chrono::steady_clock::duration run_time ) override
			{
				std::unique_lock< std::mutex > lock{ m_lock };
				if( scenario_status_t::not_started == m_status )
					{
						// Note. There is a trick: unfreezing of agents is performed
						// when scenario is locked. It means that event handlers of
						// dispatched messages will wait while that method completes.
						m_unfreezer->unfreeze();

						if( m_steps.empty() )
							m_status = scenario_status_t::completed;
						else
							{
								m_status = scenario_status_t::in_progress;
								preactivate_current_step();

								m_completion_cv.wait_for(
										lock,
										run_time,
										[this]{ return scenario_status_t::completed == m_status; });
								if( scenario_status_t::completed != m_status )
									m_status = scenario_status_t::timed_out;
							}
					}
			}

		SO_5_NODISCARD
		token_t
		pre_handler_hook(
			const incident_info_t & info ) SO_5_NOEXCEPT override
			{
				token_t result;

				std::lock_guard< std::mutex > lock{ m_lock };

				// There can be cases when pre_handler_hook is called
				// when testing scenario is already finished.
				if( scenario_status_t::in_progress == m_status &&
						m_waiting_step_index < m_steps.size() )
					{
						result = react_on_pre_handler_hook( info );
					}

				return result;
			}

		void
		post_handler_hook(
			token_t token ) SO_5_NOEXCEPT override
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				// There can be cases when post_handler_hook is called
				// when testing scenario is already finished.
				// Also we can ignore that event if token has no valid info inside.
				if( scenario_status_t::in_progress == m_status && token.valid() )
					{
						// There is a waiting step for that post_handler_hook
						// should be called.
						auto & step_to_check = token.activated_step();

						step_to_check.post_handler_hook(
								make_accessor(),
								token.step_token() );

						if( abstract_scenario_step_t::status_t::completed
								== step_to_check.status() )
							{
								m_active_steps.erase( &step_to_check );

								check_scenario_completion();
							}
					}
			}

		void
		no_handler_hook(
			const incident_info_t & info ) SO_5_NOEXCEPT override
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				// There can be cases when no_handler_hook is called
				// when testing scenario is already finished.
				if( scenario_status_t::in_progress == m_status &&
						m_waiting_step_index < m_steps.size() )
					{
						react_on_no_handler_hook( info );
					}
			}

		void
		store_state_name(
			const scenario_in_progress_accessor_t & /*accessor*/,
			const abstract_scenario_step_t & step,
			const std::string & tag,
			const std::string & state_name ) override
			{
				m_stored_states[ std::make_pair(step.name(), tag) ] = state_name;
			}

		SO_5_NODISCARD
		std::string
		stored_state_name(
			const std::string & step_name,
			const std::string & tag ) const override
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				if( scenario_status_t::completed != m_status )
					SO_5_THROW_EXCEPTION(
							rc_scenario_must_be_completed,
							"scenario must be completed before call to "
							"stored_state_name()" );

				const auto it = m_stored_states.find(
						std::make_pair(step_name, tag) );
				if( it == m_stored_states.end() )
					SO_5_THROW_EXCEPTION(
							rc_stored_state_name_not_found,
							"unable to find stored state name for <" +
							step_name + "," + tag + ">" );

				return it->second;
			}

	private :
		void
		preactivate_current_step()
			{
				m_steps[ m_waiting_step_index ]->preactivate();
			}

		SO_5_NODISCARD
		token_t
		react_on_pre_handler_hook(
			const incident_info_t & info ) SO_5_NOEXCEPT
			{
				token_t result;

				// pre_handler_hook on the current waiting step must be called.
				auto & step_to_check = *(m_steps[ m_waiting_step_index ]);
				auto step_token = step_to_check.pre_handler_hook( info );

				if( step_token.valid() )
					// Because step's token is not NULL, we should return
					// our valid token.
					// This is the case when token is necessary for the
					// subsequent post_handler_hook() call.
					result = token_t{ &step_to_check, std::move(step_token) };

				// The step can change its status...
				switch( step_to_check.status() )
					{
					case abstract_scenario_step_t::status_t::active :
						// The step is activated and we need to wait its completion.
						m_active_steps.insert( &step_to_check );
						switch_to_next_step_if_possible();
					break;

					case abstract_scenario_step_t::status_t::completed :
						// There is no need to wait the completion of the step.
						switch_to_next_step_if_possible();

						check_scenario_completion();
					break;

					case abstract_scenario_step_t::status_t::passive: break;
					case abstract_scenario_step_t::status_t::preactivated: break;
					}

				return result;
			}

		void
		react_on_no_handler_hook(
			const incident_info_t & info ) SO_5_NOEXCEPT
			{
				// no_handler_hook on the current waiting step must be called.
				auto & step_to_check = *(m_steps[ m_waiting_step_index ]);
				step_to_check.no_handler_hook( info );

				// The step can change its status...
				switch( step_to_check.status() )
					{
					case abstract_scenario_step_t::status_t::active :
						// The step is activated and we need to wait its completion.
						m_active_steps.insert( &step_to_check );
						switch_to_next_step_if_possible();
					break;

					case abstract_scenario_step_t::status_t::completed :
						// There is no need to wait the completion of the step.
						switch_to_next_step_if_possible();

						check_scenario_completion();
					break;

					case abstract_scenario_step_t::status_t::passive: break;
					case abstract_scenario_step_t::status_t::preactivated: break;
					}
			}

		void
		switch_to_next_step_if_possible()
			{
				++m_waiting_step_index;
				if( m_waiting_step_index < m_steps.size() )
					{
						preactivate_current_step();
					}
			}

		//! Checks the possibility of completion of the scenario and
		//! completes scenario if it is possible.
		void
		check_scenario_completion() SO_5_NOEXCEPT
			{
				if( m_active_steps.empty() &&
						m_waiting_step_index >= m_steps.size() )
					{
						m_status = scenario_status_t::completed;
						m_completion_cv.notify_all();
					}
			}

		SO_5_NODISCARD
		std::string
		describe_current_state() const
			{
				std::ostringstream ss;

				if( m_waiting_step_index < m_steps.size() )
					ss << "preactivated step:"
						<< m_steps[ m_waiting_step_index ]->name();
				else
					ss << "all steps handled";

				ss << ";";

				if( !m_active_steps.empty() )
					{
						ss << " active steps:{";
						bool need_comma = false;
						for( auto * s : m_active_steps )
							{
								if( need_comma )
									ss << ", ";
								else
									need_comma = true;

								ss << s->name();
							}
						ss << "};";
					}

				if( !m_stored_states.empty() )
					{
						ss << " stored states:{";
						bool need_comma = false;
						for( auto & p : m_stored_states )
							{
								if( need_comma )
									ss << ", ";
								else
									need_comma = true;

								ss << "[" << p.first.first << ", "
									<< p.first.second << "]="
									<< p.second;
							}
						ss << "};";
					}

				return ss.str();
			}
	};

} /* namespace details */

namespace impl {

namespace details = so_5::experimental::testing::v1::details;

/*!
 * \brief A special envelope that is necessary for testing scenarios.
 *
 * Testing scenario should know about every handled or rejected message
 * delivered to agents. To make this possible a trick with enveloped
 * messages are used: every message is enveloped into a special envelope.
 * This envelope has a reference to testing scenario and informs this
 * scenario when message is handled by a receiver.
 *
 * But the situation with rejected messages is not that simple.
 * We assume that if access_hook() wasn't called then message was rejected.
 * Because of that envelope has a boolean flag m_handled that is set
 * in access_hook(). This flag is analyzed in the destructor and, if it
 * is not set, we assume that message was rejected.
 *
 * In theory there can be cases when envelope is destroyed without delivery
 * to an agent. But for testing scenarios we just ignore those case.
 *
 * \since
 * v.5.5.24
 */
class special_envelope_t final : public so_5::enveloped_msg::envelope_t
	{
		//! A testing scenario for that envelope.
		outliving_reference_t< details::abstract_scenario_t > m_scenario;
		//! Information about enveloped message.
		details::incident_info_t m_demand_info;
		//! Enveloped message.
		message_ref_t m_message;
		//! Was this message handled by a receiver?
		bool m_handled;

	public :
		special_envelope_t(
			outliving_reference_t< details::abstract_scenario_t > scenario,
			const execution_demand_t & demand )
			:	m_scenario( scenario )
			,	m_demand_info( demand.m_receiver, demand.m_msg_type, demand.m_mbox_id )
			,	m_message( demand.m_message_ref )
			,	m_handled( false )
			{}
		~special_envelope_t() SO_5_NOEXCEPT override
			{
				// If message wasn't handled we assume that agent rejected
				// this message.
				if( !m_handled )
					m_scenario.get().no_handler_hook( m_demand_info );
			}

		void
		access_hook(
			//! Why this hook is called.
			access_context_t context,
			//! Proxy object which can call an actual event handler.
			handler_invoker_t & invoker ) SO_5_NOEXCEPT override
			{
				switch( context )
					{
					case access_context_t::handler_found :
						{
							// Message will be passed to event handler.
							// We assume that message is handled.
							// If we don't change m_handled flag there the
							// destructor will call no_handler_hook().
							m_handled = true;

							// We must get token...
							auto token = m_scenario.get().pre_handler_hook(
									m_demand_info );

							invoker.invoke( payload_info_t{ m_message } );

							// And now the token must be passed back.
							m_scenario.get().post_handler_hook( token );
						}
					break;

					case access_context_t::transformation :
						invoker.invoke( payload_info_t{ m_message } );
					break;

					case access_context_t::inspection :
						invoker.invoke( payload_info_t{ m_message } );
					break;
					}
			}
	};

/*!
 * \brief A mode of work for special_event_queue.
 *
 * Special event queue can work in two modes:
 * - buffered, when all messages must be stored in a local queue;
 * - direct, when all messages go directly to the original queue.
 *
 * This enumeration describes mode of operation for special_event_queue_t.
 *
 * \since
 * v.5.5.24
 */
enum class queue_mode_t
	{
		//! All messages must be stored locally.
		buffer,
		//! All messages should go to the original queue without buffering.
		direct
	};

/*!
 * \brief A special event_queue to be used for testing scenarios.
 *
 * \par Support for agents freeze
 *
 * If a testing scenario is not started yet all registered agents
 * should be in frozen state. It means that they are present in
 * the SObjectizer Environment, but do not receive nor handle any message
 * (even special message for invoking so_evt_start() method).
 *
 * To implement that special event_queue is created for every agent.
 * This event_queue works in two modes, described by
 * queue_mode_t enumeration.
 *
 * In queue_mode_t::buffer mode all messages are stored in local queue
 * and not passed to the actual event_queue created for agent by a dispatcher.
 * This way we prevent delivery of messages to agents.
 *
 * In queue_mode_t::direct mode all message are passed to the actual
 * event_queue directly, without storing them locally. In that mode
 * special event_queue works just as a proxy.
 *
 * Transfer of locally stored events is performed when mode is
 * switched from queue_mode_t::buffer to queue_mode_t::direct.
 *
 * \par Creation of special envelopes
 *
 * There is yet another task for special event_queue: all ordinary
 * messages/signals, service requests and enveloped messages must be
 * wrapped into a special envelope. This is done in push() method.
 *
 * \since
 * v.5.5.24
 */
class special_event_queue_t final : public event_queue_t
	{
		//! Object lock.
		/*!
		 * All operations will be performed under that lock.
		 */
		std::mutex m_lock;

		//! Testing scenario for that this queue was created.
		outliving_reference_t< details::abstract_scenario_t > m_scenario;
		//! Original event_queue.
		outliving_reference_t< event_queue_t > m_original_queue;

		//! The current mode of operation.
		queue_mode_t m_mode;
		//! Local storage for demands to be used in buffered mode.
		std::vector< execution_demand_t > m_buffer;

		static bool
		is_ordinary_demand( const execution_demand_t & demand ) SO_5_NOEXCEPT
			{
				return agent_t::get_demand_handler_on_message_ptr() ==
								demand.m_demand_handler ||
						agent_t::get_service_request_handler_on_message_ptr() ==
								demand.m_demand_handler ||
						agent_t::get_demand_handler_on_enveloped_msg_ptr() ==
								demand.m_demand_handler;
			}

		void
		push_to_queue( execution_demand_t demand )
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				if( queue_mode_t::buffer == m_mode )
					m_buffer.push_back( std::move(demand) );
				else
					m_original_queue.get().push( std::move(demand) );
			}

	public :
		special_event_queue_t(
			outliving_reference_t< details::abstract_scenario_t > scenario,
			outliving_reference_t< event_queue_t > original_queue,
			queue_mode_t queue_mode )
			:	m_scenario{ scenario }
			,	m_original_queue{ original_queue }
			,	m_mode{ queue_mode }
			{}

		void
		push( execution_demand_t demand ) override
			{
				if( !is_ordinary_demand( demand ) )
					// Demand must go into the original queue without transformations.
					push_to_queue( std::move(demand) );
				else
					{
						// Original message must be wrapped into a special
						// envelope and original demand must be modified.
						message_ref_t new_env{
							stdcpp::make_unique< special_envelope_t >(
									m_scenario,
									std::cref(demand) )
						};

						demand.m_message_ref = std::move(new_env);
						demand.m_demand_handler =
								agent_t::get_demand_handler_on_enveloped_msg_ptr();

						push_to_queue( std::move(demand) );
					}
			}

		void
		switch_to_direct_mode()
			{
				// Cleanup of buffer should be performed when
				// object is unlocked.
				decltype(m_buffer) tmp;
				{
					std::lock_guard< std::mutex > lock{ m_lock };

					m_mode = queue_mode_t::direct;

					for( auto & d : m_buffer )
						m_original_queue.get().push( std::move(d) );

					using std::swap;
					swap( tmp, m_buffer );
				}
			}
	};

/*!
 * \brief A special stop guard that unfreezes all agents if this
 * is not done yet.
 *
 * There can be cases when testing environment is started, some
 * agents are registered, but testing scenario is not started. For example
 * the work of test case is cancelled by an exception.
 *
 * In those cases testing environment should be correctly stopped.
 * All agents have to be deregistered. We can do that only if they
 * is not frozen. But they are frozen.
 *
 * This special stop_guard is intended to unfreeze all agents if
 * testing environment finishes its work without launching the
 * testing scenario.
 *
 * \since
 * v.5.5.24
 */
class stop_guard_for_unfreezer_t final
	:	public stop_guard_t
	,	public std::enable_shared_from_this< stop_guard_for_unfreezer_t >
	{
		outliving_reference_t< details::agent_unfreezer_t > m_unfreezer;
		outliving_reference_t< environment_t > m_env;

	public :
		stop_guard_for_unfreezer_t(
			outliving_reference_t< details::agent_unfreezer_t > unfreezer,
			outliving_reference_t< environment_t > env )
			:	m_unfreezer{ unfreezer }
			,	m_env{ env }
			{}

		void
		stop() SO_5_NOEXCEPT override
			{
				so_5::details::invoke_noexcept_code( [this] {
					// Agents should become unfrozen.
					m_unfreezer.get().unfreeze();

					// We should remove itself to allow the environment
					// to continue shutdown operation.
					m_env.get().remove_stop_guard( shared_from_this() );
				} );
			}
	};

/*!
 * \brief A special event_queue_hook that creates instances of
 * special event_queue for testing scenario.
 *
 * This type is also an implementation of agent_unfreezer_t interface.
 *
 * To implement unfreezing of agents object of this types stores pointers
 * to all created event_queue in internal container. In unfreeze()
 * method all those queues are switched from buffered to direct mode.
 *
 * \since
 * v.5.5.24
 */
class special_event_queue_hook_t final
	:	public event_queue_hook_t
	,	public details::agent_unfreezer_t
	{
		//! Lock for this object.
		std::mutex m_lock;

		//! Mode of operation for new queues.
		queue_mode_t m_mode{ queue_mode_t::buffer };

		//! Testing scenario for that this object is created.
		outliving_reference_t< details::abstract_scenario_t > m_scenario;

		//! List of all queues created before unfreeze was called.
		/*!
		 * If unfreeze() was called then pointers to new event queues
		 * are not stored in that list.
		 *
		 * \note
		 * Items from this containers are not removed in on_unbind method.
		 * We assume that agents can't be deregistered when all agents
		 * are frozen.
		 */
		std::vector< special_event_queue_t * > m_created_queues;

	public :
		special_event_queue_hook_t(
			outliving_reference_t< details::abstract_scenario_t > scenario )
			:	m_scenario{ scenario }
			{}

		SO_5_NODISCARD
		event_queue_t *
		on_bind(
			agent_t * /*agent*/,
			event_queue_t * original_queue ) SO_5_NOEXCEPT override
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				auto sq = so_5::stdcpp::make_unique< special_event_queue_t >(
						m_scenario,
						outliving_mutable( *original_queue ),
						m_mode );

				if( queue_mode_t::buffer == m_mode )
					m_created_queues.push_back( sq.get() );

				return sq.release();
			}

		void
		on_unbind(
			agent_t * /*agent*/,
			event_queue_t * queue ) SO_5_NOEXCEPT override
			{
				// Assume that queue is an instance of special_event_queue_t
				// and simply delete it.
				delete queue;
			}

		void
		unfreeze() SO_5_NOEXCEPT override
			{
				// Mode will be switched under locked mutex.
				// But actual switching of queue operation mode
				// for already created queue will be performed with
				// unlocked mutex.
				decltype(m_created_queues) tmp;
				{
					std::lock_guard< std::mutex > lock{ m_lock };

					m_mode = queue_mode_t::direct;

					using std::swap;
					swap( tmp, m_created_queues );
				}

				// Now we can switch mode for all queues those were created
				// in buffered mode.
				for( auto * sq : tmp )
					sq->switch_to_direct_mode();
			}
	};

/*!
 * \brief A helper object for synchronization between helper worker
 * there where testing environment is launched and user thread.
 *
 * We must stop work of testing_env_t constructor until
 * testing_env_t::tune_environment_on_start() finishes its work.
 * To do that constructor will wait on a future object for that
 * promise, and tune_environment_on_start() will set this promise.
 *
 * \since
 * v.5.5.24
 */
struct init_completed_data_t
	{
		std::promise<void> m_completed;
	};

} /* namespace impl */

/*!
 * \brief Internal data for testing environment.
 *
 * \attention
 * real_scenario object must be declared before special_hook object.
 */
struct testing_env_t::internals_t
	{
		details::real_scenario_t m_scenario;
		impl::special_event_queue_hook_t m_special_hook;

		impl::init_completed_data_t m_init_completed;

		internals_t()
			:	m_special_hook{ outliving_mutable(m_scenario) }
			{
				m_scenario.setup_unfreezer( m_special_hook );
			}

		SO_5_NODISCARD
		static std::unique_ptr< internals_t >
		make() { return so_5::stdcpp::make_unique<internals_t>(); }
	};

namespace impl {

void
setup_special_queue_hook(
	testing_env_t::internals_t & internals,
	environment_params_t & to )
	{
		to.event_queue_hook( event_queue_hook_unique_ptr_t{
				&internals.m_special_hook,
				event_queue_hook_t::noop_deleter } );
	}

SO_5_NODISCARD
environment_params_t
make_tuned_params(
	so_5::api::generic_simple_so_env_params_tuner_t env_params_tuner )
	{
		environment_params_t result;
		env_params_tuner( result );

		return result;
	}

SO_5_NODISCARD
environment_params_t
make_special_params(
	testing_env_t::internals_t & internals,
	environment_params_t && params )
	{
		setup_special_queue_hook( internals, params );

		return std::move(params);
	}

} /* namespace impl */

//
// scenario_proxy_t
//
scenario_proxy_t::scenario_proxy_t(
	outliving_reference_t< details::abstract_scenario_t > scenario )
	:	m_scenario{ scenario }
	{}

SO_5_NODISCARD
step_definition_proxy_t
scenario_proxy_t::define_step(
	nonempty_name_t step_name )
	{
		return m_scenario.get().define_step( std::move(step_name) );
	}

SO_5_NODISCARD
scenario_result_t
scenario_proxy_t::result() const
	{
		return m_scenario.get().result();
	}

void
scenario_proxy_t::run_for(
	std::chrono::steady_clock::duration run_time )
	{
		return m_scenario.get().run_for( run_time );
	}

SO_5_NODISCARD
std::string
scenario_proxy_t::stored_state_name(
	const std::string & step_name,
	const std::string & tag ) const
	{
		return m_scenario.get().stored_state_name( step_name, tag );
	}

//
// testing_env_t
//
testing_env_t::testing_env_t()
	:	testing_env_t{ environment_params_t{} }
	{}

testing_env_t::testing_env_t(
	so_5::api::generic_simple_so_env_params_tuner_t env_params_tuner )
	:	testing_env_t{ impl::make_tuned_params( std::move(env_params_tuner) ) }
	{}

testing_env_t::testing_env_t(
	environment_params_t && env_params )
	:	m_internals{ internals_t::make() }
	,	m_sobjectizer{
			[this]( environment_t & env ) { tune_environment_on_start( env ); },
			impl::make_special_params(
					outliving_mutable( *m_internals ),
					std::forward<environment_params_t>(env_params) ) }
	{
		// We must wait completion of tune_environment_on_start().
		wait_init_completion();
	}

testing_env_t::~testing_env_t()
	{}

environment_t &
testing_env_t::environment() const
	{
		return m_sobjectizer.environment();
	}

void
testing_env_t::stop()
	{
		m_sobjectizer.stop();
	}

void
testing_env_t::join()
	{
		m_sobjectizer.join();
	}

void
testing_env_t::stop_then_join()
	{
		m_sobjectizer.stop_then_join();
	}

SO_5_NODISCARD
scenario_proxy_t
testing_env_t::scenario() SO_5_NOEXCEPT
	{
		return { outliving_mutable(m_internals->m_scenario) };
	}

void
testing_env_t::tune_environment_on_start( environment_t & env )
	{
		// stop_guard for unfreezing agents must be installed before
		// an user will start work with the environment.
		env.setup_stop_guard(
				std::make_shared< impl::stop_guard_for_unfreezer_t >(
						outliving_mutable(m_internals->m_special_hook),
						outliving_mutable(env) ) );

		// This action was done on separate thread.
		// The constructor of testing_env_t waits completion of
		// this method on another thread.
		// Do allow the constructor to complete its work.
		m_internals->m_init_completed.m_completed.set_value();
	}

void
testing_env_t::wait_init_completion()
	{
		m_internals->m_init_completed.m_completed.get_future().wait();
	}

} /* namespace v1 */

} /* namespace testing */

} /* namespace experimental */

} /* namespace so_5 */

