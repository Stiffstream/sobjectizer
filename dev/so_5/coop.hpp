/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Cooperation of agents.
 * 
 * \since
 * v.5.6.0
 */

#pragma once

#include <so_5/compiler_features.hpp>
#include <so_5/declspec.hpp>
#include <so_5/exception.hpp>
#include <so_5/types.hpp>

#include <so_5/coop_handle.hpp>
#include <so_5/agent.hpp>
#include <so_5/disp_binder.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <string>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

namespace dereg_reason
{

/*!
 * \name Cooperation deregistration reasons.
 * \{
 */
//! Normal deregistration.
const int normal = 0;

//! Deregistration because SObjectizer Environment shutdown.
const int shutdown = 1;

//! Deregistration because parent cooperation deregistration.
const int parent_deregistration = 2;

//! Deregistration because of unhandled exception.
const int unhandled_exception = 3;

//! Deregistration because of unknown error.
const int unknown_error = 4;

//! Reason is not properly defined.
const int undefined = -1;

//! A starting point for user-defined reasons.
const int user_defined_reason = 0x1000;
/*!
 * \}
 */

} /* namespace dereg_reason */

//
// coop_dereg_reason_t
//
/*!
 * \since
 * v.5.2.3
 *
 */
class coop_dereg_reason_t
	{
	public :
		coop_dereg_reason_t() noexcept
			:	m_reason( dereg_reason::undefined )
			{}

		explicit coop_dereg_reason_t( int reason ) noexcept
			:	m_reason( reason )
			{}

		int
		reason() const noexcept { return m_reason; }

	private :
		int m_reason;
	};

//
// coop_reg_notificator_t
//
//FIXME: coop notificator should be a noexcept function.
/*!
 * \since
 * v.5.2.3
 *
 * \brief Type of cooperation registration notificator.
 *
 * Cooperation notificator should be a function with the following
 * prototype:
\code
void
notificator(
	// SObjectizer Environment for cooperation.
	so_5::environment_t & env,
	// Coop's handle.
	const coop_handle_t & coop );
\endcode
 */
using coop_reg_notificator_t =
		std::function< void(environment_t &, const coop_handle_t &) >;

//
// coop_reg_notificators_container_t
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief Container for cooperation registration notificators.
 */
class SO_5_TYPE coop_reg_notificators_container_t
	:	public atomic_refcounted_t
{
	public :
		//! Add a notificator.
		void
		add(
			coop_reg_notificator_t notificator )
		{
			m_notificators.push_back( std::move(notificator) );
		}

		//! Call all notificators.
		void
		call_all(
			environment_t & env,
			const coop_handle_t & coop ) const noexcept;

	private :
		std::vector< coop_reg_notificator_t > m_notificators;
};

//FIXME: is this level of indirection really needed now?
//Maybe coop_reg_notificators_container_t can just be a field of coop_t?

//
// coop_reg_notificators_container_ref_t
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief Typedef for smart pointer to notificators_container.
 */
using coop_reg_notificators_container_ref_t =
		intrusive_ptr_t< coop_reg_notificators_container_t >;

//
// coop_dereg_notificator_t
//
//FIXME: coop notificator should be a noexcept function.
/*!
 * \since
 * v.5.2.3
 *
 * \brief Type of cooperation deregistration notificator.
 *
 * Cooperation notificator should be a function with the following
 * prototype:
\code
void
notificator(
	// SObjectizer Environment for cooperation.
	so_5::environment_t & env,
	// Coop's handle.
	const coop_handle_t & coop,
	// Reason of deregistration.
	const so_5::coop_dereg_reason_t & reason );
\endcode
 */
using coop_dereg_notificator_t = std::function<
		void(
				environment_t &,
				const coop_handle_t &,
				const coop_dereg_reason_t &) >;

//
// coop_dereg_notificators_container_t
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief Container for cooperation deregistration notificators.
 */
class SO_5_TYPE coop_dereg_notificators_container_t
	:	public atomic_refcounted_t
{
	public :
		//! Add a notificator.
		void
		add(
			coop_dereg_notificator_t notificator )
		{
			m_notificators.push_back( std::move(notificator) );
		}

		//! Call all notificators.
		void
		call_all(
			environment_t & env,
			const coop_handle_t & coop,
			const coop_dereg_reason_t & reason ) const noexcept;

	private :
		std::vector< coop_dereg_notificator_t > m_notificators;
};

//FIXME: is this level of indirection really needed now?
//Maybe coop_dereg_notificators_container_t can just be a field of coop_t?

//
// coop_dereg_notificators_container_ref_t
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief Typedef for smart pointer to notificators_container.
 */
using coop_dereg_notificators_container_ref_t =
		intrusive_ptr_t< coop_dereg_notificators_container_t >;

namespace impl
{

class coop_private_iface_t;

//FIXME: document this!
class SO_5_TYPE coop_impl_t
	{
		friend class so_5::coop_t;
		friend class so_5::impl::coop_private_iface_t;

		//! Perform all necessary cleanup actions for coop.
		static void
		destroy_content(
			//! Target coop.
			coop_t & coop ) noexcept;

		//! Add agent to cooperation.
		/*!
		 * Cooperation takes care about agent lifetime.
		 *
		 * Default dispatcher binding is used for the agent.
		 */
		static void
		do_add_agent(
			//! Target coop.
			coop_t & coop,
			//! Agent to be bound to the coop.
			agent_ref_t agent_ref );

		//! Add agent to the cooperation with the dispatcher binding.
		/*!
		 * Instead of the default dispatcher binding the \a disp_binder
		 * is used for this agent during the cooperation registration.
		 */
		static void
		do_add_agent(
			//! Target coop.
			coop_t & coop,
			//! Agent.
			agent_ref_t agent_ref,
			//! Agent to dispatcher binder.
			disp_binder_shptr_t disp_binder );

		//! Add notificator about cooperation registration event.
		static void
		add_reg_notificator(
			//! Target coop.
			coop_t & coop,
			//! Notificator to be added.
			coop_reg_notificator_t notificator );

		//! Add notificator about cooperation deregistration event.
		static void
		add_dereg_notificator(
			//! Target coop.
			coop_t & coop,
			//! Notificator to be added.
			coop_dereg_notificator_t notificator );

		//! Get exception reaction for coop.
		SO_5_NODISCARD
		static exception_reaction_t
		exception_reaction(
			//! Target coop.
			const coop_t & coop ) noexcept;

		//! Do decrement reference count for a coop.
		static void
		do_decrement_reference_count(
			//! Target coop.
			coop_t & coop );

		//! Perform actions related to the registration of coop.
		static void
		do_registration_specific_actions( coop_t & coop );

		class registration_performer_t;

		//! Perform addition of a new child coop.
		static void
		do_add_child(
			//! Parent coop.
			coop_t & parent,
			//! Child to be added.
			coop_shptr_t child );
	};

} /* namespace impl */

//! Agent cooperation.
/*!
 * The main purpose of the cooperation is the introducing of several agents into
 * SObjectizer as a single unit. A cooperation should be registered.
 *
 * For the cooperation to be successfuly registered all of its agents must 
 * successfuly pass registration steps (so-define, bind to the dispatcher). 
 * If at least one agent out of this cooperation fails to pass any of 
 * mentioned steps, the cooperation will not be registered and 
 * all of agents will run procedures opposite to registration 
 * steps (unbind from the dispatcher, so-undefine) which had been successfuly 
 * taken for the particulary agent in the reverse order.
 *
 * Agents are added to the cooperation by the add_agent() method.
 *
 * After addition to the cooperation the cooperation takes care about
 * the agent lifetime.
 */
class coop_t : public std::enable_shared_from_this<coop_t>
{
	private :
		friend class agent_t;
		friend class impl::coop_private_iface_t;
		friend class impl::coop_impl_t;
		friend class impl::coop_impl_t::registration_performer_t;

		coop_t( const coop_t & ) = delete;
		coop_t & operator=( const coop_t & ) = delete;
		coop_t( coop_t && ) = delete;
		coop_t & operator=( coop_t && ) = delete;

	protected :
		virtual ~coop_t() 
			{
				impl::coop_impl_t::destroy_content( *this );
			}

		//! Constructor.
		coop_t(
			//! Cooperation ID.
			coop_id_t id,
			//! Parent coop.
			coop_handle_t parent,
			//! Default dispatcher binding.
			disp_binder_shptr_t coop_disp_binder,
			//! SObjectizer Environment.
			outliving_reference_t< environment_t > env )
			:	m_id{ id }
			,	m_parent{ std::move(parent) }
			,	m_coop_disp_binder{ std::move(coop_disp_binder) }
			,	m_env{ env }
			{}

	public:
		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Deleter for agent_coop.
		 */
		static void
		destroy( coop_t * coop ) { delete coop; }

		/*!
		 * \brief Get handle for this coop.
		 *
		 * \since
		 * v.5.6.0
		 */
		coop_handle_t
		handle() noexcept
			{
				return coop_handle_t{ m_id, shared_from_this() };
			}

		/*!
		 * \brief Get the ID of coop.
		 *
		 * \since
		 * v.5.6.0
		 */
		coop_id_t
		id() const noexcept { return m_id; }

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief Access to SO Environment for which cooperation is bound.
		 */
		environment_t &
		environment() const noexcept
			{
				return m_env.get();
			}

		//! Add agent to cooperation.
		/*!
		 * Cooperation takes care about agent lifetime.
		 *
		 * Default dispatcher binding is used for the agent.
		 */
		template< class Agent >
		inline Agent *
		add_agent(
			//! Agent.
			std::unique_ptr< Agent > agent )
			{
				Agent * p = agent.get();

				impl::coop_impl_t::do_add_agent(
						*this,
						agent_ref_t{ std::move(agent) } );

				return p;
			}

		//! Add agent to the cooperation with the dispatcher binding.
		/*!
		 * Instead of the default dispatcher binding the \a disp_binder
		 * is used for this agent during the cooperation registration.
		 */
		template< class Agent >
		inline Agent *
		add_agent(
			//! Agent.
			std::unique_ptr< Agent > agent,
			//! Agent to dispatcher binder.
			disp_binder_shptr_t disp_binder )
			{
				Agent * p = agent.get();

				impl::coop_impl_t::do_add_agent(
					*this,
					agent_ref_t{ std::move(agent) },
					std::move(disp_binder) );

				return p;
			}

		/*!
		 * \name Method for working with notificators.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Add notificator about cooperation registration event.
		 */
		void
		add_reg_notificator(
			coop_reg_notificator_t notificator )
			{
				impl::coop_impl_t::add_reg_notificator(
						*this,
						std::move(notificator) );
			}

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Add notificator about cooperation deregistration event.
		 */
		void
		add_dereg_notificator(
			coop_dereg_notificator_t notificator )
			{
				impl::coop_impl_t::add_dereg_notificator(
						*this,
						std::move(notificator) );
			}
		/*!
		 * \}
		 */

		/*!
		 * \name Method for working with user resources.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Take a user resouce under cooperation control.
		 */
		template< class T >
		T *
		take_under_control( std::unique_ptr< T > resource )
			{
				T * ret_value = resource.get();

//FIXME: a custom deleter object of fixed size should be used here
//instead of std::function!
				resource_deleter_t d = [ret_value]() { delete ret_value; };
				m_resource_deleters.emplace_back( std::move(d) );

				resource.release();

				return ret_value;
			}
		/*!
		 * \}
		 */

		/*!
		 * \name Exception reaction methods.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief Set exception reaction for that cooperation.
		 *
		 * This value will be used by agents and children cooperation if
		 * they use inherit_exception_reaction value.
		 */
		void
		set_exception_reaction(
			exception_reaction_t value ) noexcept
			{
				m_exception_reaction = value;
			}

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief Get the current exception rection flag for that cooperation.
		 *
		 * It uses the following logic:
		 * - if own's exception_reaction flag value differs from
		 *   inherit_exception_reaction value than own's exception_reaction
		 *   flag value returned;
		 * - otherwise if there is a parent cooperation than parent coop's
		 *   exception_reaction value returned;
		 * - otherwise SO Environment's exception_reaction is returned.
		 */
		SO_5_NODISCARD
		exception_reaction_t
		exception_reaction() const noexcept
			{
				return impl::coop_impl_t::exception_reaction( *this );
			}
		/*!
		 * \}
		 */

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Helper method for simplification of agents creation.
		 *
		 * \note Creates an instance of agent of type \a Agent and adds it to
		 * the cooperation.
		 *
		 * \return pointer to the new agent.
		 *
		 * \tparam Agent type of agent to be created.
		 * \tparam Args type of parameters list for agent constructor.
		 *
		 * \par Usage sample:
		 \code
		 so_5::coop_unique_ptr_t coop = env.create_coop( so_5::autoname );
		 // For the case of constructor like my_agent(environmen_t&).
		 coop->make_agent< my_agent >(); 
		 // For the case of constructor like your_agent(environment_t&, std::string).
		 auto ya = coop->make_agent< your_agent >( "hello" );
		 // For the case of constructor like thier_agent(environment_t&, std::string, mbox_t).
		 coop->make_agent< their_agent >( "bye", ya->so_direct_mbox() );
		 \endcode
		 */
		template< class Agent, typename... Args >
		Agent *
		make_agent( Args &&... args )
			{
				return this->add_agent(
						std::make_unique<Agent>(
								environment(), std::forward<Args>(args)...) );
			}

//FIXME: check the correctness of this doxygen comment!
		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Helper method for simplification of agents creation and
		 * binding to the specified dispatcher.
		 *
		 * \note Creates an instance of agent of type \a Agent and adds it to
		 * the cooperation with the specified binder.
		 *
		 * \return pointer to the new agent.
		 *
		 * \tparam Agent type of agent to be created.
		 * \tparam Args type of parameters list for agent constructor.
		 *
		 * \par Usage sample:
		 \code
		 so_5::disp::one_thread::private_dispatcher_handler_t disp =
		 		so_5::disp::one_thread::create_private_disp();
		 so_5::coop_unique_ptr_t coop = env.create_coop( so_5::autoname );
		 // For the case of constructor like my_agent(environmen_t&).
		 coop->make_agent_with_binder< my_agent >( disp->binder() ); 
		 // For the case of constructor like your_agent(environment_t&, std::string).
		 auto ya = coop->make_agent_with_binder< your_agent >( disp->binder(), "hello" );
		 // For the case of constructor like thier_agent(environment_t&, std::string, mbox_t).
		 coop->make_agent_with_binder< their_agent >( disp->binder(), "bye", ya->so_direct_mbox() );
		 \endcode
		 */
		template< class Agent, typename... Args >
		Agent *
		make_agent_with_binder(
			//! A dispatcher binder for the new agent.
			so_5::disp_binder_shptr_t binder,
			//! Arguments to be passed to the agent's constructor.
			Args &&... args )
			{
				return this->add_agent(
						std::make_unique<Agent>(
								environment(), std::forward<Args>(args)...),
						std::move(binder) );
			}

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Get agent count in the cooperation.
		 */
		std::size_t
		query_agent_count() const noexcept
			{
				return m_agent_array.size();
			}

		/*!
		 * \since
		 * v.5.5.16
		 *
		 * \brief Get agent count in the cooperation.
		 * \note Just an alias for query_agent_count().
		 */
		std::size_t
		size() const noexcept { return query_agent_count(); }

		/*!
		 * \since
		 * v.5.5.16
		 *
		 * \brief Get the capacity of vector for holding agents list.
		 */
		std::size_t
		capacity() const noexcept { return m_agent_array.capacity(); }

		/*!
		 * \since
		 * v.5.5.16
		 *
		 * \brief Reserve a space for vector for holding agents list.
		 *
		 * This method can help avoid reallocations of agents list during
		 * filling the coop:
		 * \code
			env.introduce_coop( []( so_5::coop_t & coop ) {
				coop.reserve( agents_count );
				for( size_t i = 0; i != agents_count; ++i )
					coop.make_agent< some_type >( some_args );
			} );
		 * \endcode
		 */
		void
		reserve( std::size_t v ) { m_agent_array.reserve( v ); }
		
#if 0
		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Deregister the cooperation with the specified reason.
		 *
		 * \par Usage example:
			\code
			so_5::environment_t & env = ...;
			env.introduce_coop( []( so_5::coop_t & coop ) {
					coop.define_agent()
						.event< some_signal >( some_mbox, [&coop] {
							...
							coop.deregister( so_4::dereg_reason::user_defined_reason + 100 );
						} );
				} );
			\endcode
		 *
		 * \note This method is just a shorthand for:
			\code
			so_5::coop_t & coop = ...;
			coop.environment().deregister_coop( coop.query_coop_name(), reason );
			\endcode
		 */
		void
		deregister(
			//! Reason of cooperation deregistration.
			int reason );

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Deregistr the cooperation normally.
		 *
		 * \note This method is just a shorthand for:
			\code
			so_5::coop_t & coop = ...;
			coop.deregister( so_5::dereg_reason::normal );
			\endcode
		 */
		inline void
		deregister_normally()
			{
				this->deregister( dereg_reason::normal );
			}
#endif

	protected:
		//! Information about agent and its dispatcher binding.
		struct agent_with_disp_binder_t
		{
			agent_with_disp_binder_t(
				agent_ref_t agent_ref,
				disp_binder_shptr_t binder )
				:	m_agent_ref{ std::move(agent_ref) }
				,	m_binder{ std::move(binder) }
			{}

			//! Agent.
			agent_ref_t m_agent_ref;

			//! Agent to dispatcher binder.
			disp_binder_shptr_t m_binder;
		};

		//! Typedef for the agent information container.
		using agent_array_t = std::vector< agent_with_disp_binder_t >;

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Registration status.
		 */
		enum class registration_status_t
		{
			//! Cooperation is not registered yet.
			coop_not_registered,
			//! Cooperation is registered.
			/*!
			 * Reference count for cooperation in that state should
			 * be greater than zero.
			 */
			coop_registered,
			//! Cooperation is in deregistration process.
			/*!
			 * Reference count for cooperation in that state should
			 * be zero.
			 */
			coop_deregistering
		};

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Type of user resource deleter.
		 */
		using resource_deleter_t = std::function< void() >;

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Type of container for user resource deleters.
		 */
		using resource_deleter_vector_t = std::vector< resource_deleter_t >;

		//! Cooperation ID.
		const coop_id_t m_id;

		//! Parent coop.
		/*!
		 * \note
		 * This handle will be empty only for special root-coop.
		 * All other coops will have an actual parent (sometimes in the
		 * form of unvisible to user root-coop).
		 */
		coop_handle_t m_parent;

		//! Default agent to the dispatcher binder.
		disp_binder_shptr_t m_coop_disp_binder;

		//! Cooperation agents.
		agent_array_t m_agent_array;

		//! SObjectizer Environment for which cooperation is created.
		outliving_reference_t< environment_t > m_env;

		//! Count for entities.
		/*!
		 * Since v.5.2.3 this counter includes:
		 * - count of agents from cooperation;
		 * - count of direct child cooperations;
		 * - usage of cooperation pointer in cooperation registration routine.
		 *
		 * \sa coop_t::increment_usage_count()
		 */
		atomic_counter_t m_reference_count{ 0l };

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Notificators for registration event.
		 */
		coop_reg_notificators_container_ref_t m_reg_notificators;

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Notificators for deregistration event.
		 */
		coop_dereg_notificators_container_ref_t m_dereg_notificators;

		/*!
		 * \brief A lock for synchonization of some operations on coop.
		 *
		 * A new way of handling coop registration was introduced
		 * in v.5.5.8. Agents from the coop cannot start its work before
		 * finish of main coop registration actions (expecialy before
		 * end of such important step as binding agents to dispatchers).
		 * But some agents could receive evt_start event before end of
		 * agents binding step. Those agents will be stopped on this lock.
		 *
		 * Coop acquires this lock before agents binding step and releases
		 * just after that. Every agent tries to acquire it during handling
		 * of evt_start. If lock is belong to coop then an agent will be stopped
		 * until the lock will be released.
		 *
		 * Since v.5.6.0 this lock is also used for protecting changes of
		 * coop's status and sibling's chain.
		 */
		std::mutex m_lock;
		
		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief The registration status of cooperation.
		 *
		 * By default cooperation has NOT_REGISTERED status.
		 * It is changed to REGISTERED after successfull completion
		 * of all registration-specific actions.
		 *
		 * And then changed to DEREGISTERING when m_reference_count
		 * becames zero and final deregistration demand would be
		 * put to deregistration thread.
		 */
		registration_status_t m_registration_status{
				registration_status_t::coop_not_registered };

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Container of user resource deleters.
		 */
		resource_deleter_vector_t m_resource_deleters;

//FIXME: actualize comment after implementation of deregistration of coop.
		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Deregistration reason.
		 *
		 * Receives actual value only in do_deregistration_specific_actions().
		 */
		coop_dereg_reason_t m_dereg_reason;

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief A reaction to non-handled exception.
		 *
		 * By default inherit_exception_reaction is used. It means that
		 * actual exception reaction should be provided by parent coop
		 * or by SO Environment.
		 */
		exception_reaction_t m_exception_reaction{
				inherit_exception_reaction };

		/*!
		 * \brief The head of list of children coops.
		 *
		 * \since
		 * v.5.6.0
		 */
		coop_shptr_t m_first_child;

		/*!
		 * \brief The previous coop in sibling's chain.
		 *
		 * Can be nullptr if this coop is the first coop in the chain.
		 *
		 * \note
		 * This field will be used only by parent coop.
		 *
		 * \since
		 * v.5.6.0
		 */
		coop_shptr_t m_prev_sibling;

		/*!
		 * \brief The next coop in sibling's chain.
		 *
		 * Can be nullptr if this coop is the last coop in the chain.
		 *
		 * \note
		 * This field will be used only by parent coop.
		 *
		 * \since
		 * v.5.6.0
		 */
		coop_shptr_t m_next_sibling;

		/*!
		 * \brief Increment usage count for the coop.
		 */
		void
		increment_usage_count() noexcept
			{
				++m_reference_count;
			}

		/*!
		 * \brief Decrement usage count for the coop.
		 *
		 * Note that is usage count is become 0 then final deregistration
		 * actions will be initiated.
		 */
		void
		decrement_usage_count()
			{
				impl::coop_impl_t::do_decrement_reference_count( *this );
			}

		/*!
		 * \brief Add a new child to the parent coop.
		 *
		 * This method is called by child coop.
		 *
		 * \note
		 * This method locks the parent coop object. But under that lock
		 * fields m_prev_sibling and m_next_sibling of the child coop
		 * are modified.
		 *
		 * \since
		 * v.5.6.0
		 */
		void
		add_child(
			//! Child coop to be added.
			coop_shptr_t child )
			{
				impl::coop_impl_t::do_add_child( *this, std::move(child) );
			}

#if 0


		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Perform all neccessary actions related to
		 * cooperation registration.
		 */
		void
		do_registration_specific_actions(
			//! Pointer to the parent cooperation.
			//! Contains nullptr if there is no parent cooperation.
			coop_t * agent_coop );

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Perform all necessary actions related to
		 * cooperation deregistration.
		 */
		void
		do_deregistration_specific_actions(
			//! Deregistration reason.
			coop_dereg_reason_t dereg_reason );

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Rearrangement of agents in agents storage with
		 * respect to its priorities.
		 *
		 * This step is necessary to handle agents with high priorities
		 * before agents with low priorities.
		 */
		void
		reorder_agents_with_respect_to_priorities();

		//! Bind agents to the cooperation.
		void
		bind_agents_to_coop();

		//! Calls define_agent method for all cooperation agents.
		void
		define_all_agents();

		//! Bind agents to the dispatcher.
		void
		bind_agents_to_disp();

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Shutdown all agents as a part of cooperation deregistration.
		 *
		 * An exception from agent_t::shutdown_agent() leads to call to abort().
		 */
		void
		shutdown_all_agents();

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Increment usage counter for this cooperation.
		 *
		 * In v.5.2.3 the counter m_reference_count is used to
		 * reflect count of references to the cooperation. There are
		 * the following entities who can refer to cooperation:
		 * - agents from that cooperation. When cooperation is successfully
		 *   registered the counter is incremented by count of agents.
		 *   During cooperation deregistration agents finish their work and
		 *   each agent decrement cooperation usage counter;
		 * - children cooperations. Each child cooperation increments
		 *   reference counter on its registration and decrements counter
		 *   on its deregistration;
		 * - cooperation registration routine. It increment reference counter
		 *   to prevent cooperation deregistration before the end of
		 *   registration process. It is possible if cooperation do its
		 *   work very quickly and initiates deregistration. When cooperation
		 *   has coop_notificators its registration process may be longer
		 *   then cooperation working time. And cooperation could be
		 *   deregistered and even destroyed before return from registration
		 *   routine. To prevent this cooperation registration routine
		 *   increments cooperation usage counter and the begin of process
		 *   and decrement it when registration process finished.
		 */
		void
		increment_usage_count();

		//! Process signal about finished work of an agent or
		//! child cooperation.
		/*!
		 * Cooperation deregistration is a long process. All agents
		 * process events out of their queues. When an agent detects that
		 * no more events in its queue it informs the cooperation about this.
		 *
		 * When cooperation detects that all agents have finished their
		 * work it initiates the agent's destruction.
		 *
		 * Since v.5.2.3 this method used not only for agents of cooperation
		 * but and for children cooperations. Because final step of
		 * cooperation deregistration could be initiated only when all
		 * children cooperations are deregistered and destroyed.
		 */
		void
		decrement_usage_count();

		//! Do the final deregistration stage.
		void
		final_deregister_coop();

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Get pointer to the parent cooperation.
		 *
		 * \retval nullptr if there is no parent cooperation.
		 */
		coop_t *
		parent_coop_ptr() const;

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Get registration notificators.
		 */
		coop_reg_notificators_container_ref_t
		reg_notificators() const;

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Get deregistration notificators.
		 */
		coop_dereg_notificators_container_ref_t
		dereg_notificators() const;

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Delete all user resources.
		 */
		void
		delete_user_resources();

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Get deregistration reason.
		 */
		const coop_dereg_reason_t &
		dereg_reason() const;
#endif

};

/*!
 * \since
 * v.5.2.3
 *
 * \brief A custom deleter for cooperation.
 */
class coop_deleter_t
{
	public :
		inline void
		operator()( coop_t * coop ) { coop_t::destroy( coop ); }
};

//! Typedef for the agent_coop autopointer.
using coop_unique_ptr_t = std::unique_ptr< coop_t, coop_deleter_t >;

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

