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
#include <type_traits>

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

//
// coop_impl_t
//
/*!
 * \brief An internal class with real implementation of coop's logic.
 *
 * Class coop_t is derived from std::enable_shared_from_this. But when
 * coop_t is exported from a DLL the VC++ compiler issue some warnings
 * about dll-linkage of std::enable_shared_from_this. To avoid these
 * warnings coop_t is just a colletion of data. All coop's logic is
 * implemented by coop_impl_t.
 *
 * \since
 * v.5.6.0
 */
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
		[[nodiscard]]
		static exception_reaction_t
		exception_reaction(
			//! Target coop.
			const coop_t & coop ) noexcept;

		//! Do decrement reference count for a coop.
		/*!
		 * \note
		 * This method is marked as noexcept because there is no way
		 * to recover if any exception is raised here.
		 */
		static void
		do_decrement_reference_count(
			//! Target coop.
			coop_t & coop ) noexcept;

		//! Perform actions related to the registration of coop.
		static void
		do_registration_specific_actions( coop_t & coop );

		class registration_performer_t;

		//! Perform actions related to the deregistration of coop.
		/*!
		 * \note
		 * This method is marked as noexcept because there is no way
		 * to recover if any exception is raised here.
		 */
		static void
		do_deregistration_specific_actions(
			//! Coop to be deregistered.
			coop_t & coop,
			//! Reason of coop's deregistration.
			coop_dereg_reason_t reason ) noexcept;

		class deregistration_performer_t;

		//! Perform final deregistration actions for an coop.
		static void
		do_final_deregistration_actions(
			//! Target coop.
			coop_t & coop );

		//! Perform addition of a new child coop.
		static void
		do_add_child(
			//! Parent coop.
			coop_t & parent,
			//! Child to be added.
			coop_shptr_t child );

		//! Perform removement of a child coop.
		static void
		do_remove_child(
			//! Parent coop.
			coop_t & parent,
			//! Child to be removed.
			coop_t & child );
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
		friend class impl::coop_impl_t::deregistration_performer_t;

		coop_t( const coop_t & ) = delete;
		coop_t & operator=( const coop_t & ) = delete;
		coop_t( coop_t && ) = delete;
		coop_t & operator=( coop_t && ) = delete;

	protected :
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
		virtual ~coop_t() 
			{
				impl::coop_impl_t::destroy_content( *this );
			}

		/*!
		 * \brief Get handle for this coop.
		 *
		 * \since
		 * v.5.6.0
		 */
		[[nodiscard]]
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
		[[nodiscard]]
		coop_id_t
		id() const noexcept { return m_id; }

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief Access to SO Environment for which cooperation is bound.
		 */
		[[nodiscard]]
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
		 *
		 * Notificator should be a function or functional object with the
		 * following signature:
		 * \code
		 * void(so_5::environment_t & env, const so_5::coop_handle_t & handle) noexcept;
		 * \endcode
		 *
		 * An example:
		 * \code
		 * class parent_agent final : public so_5::agent_t
		 * {
		 * 	// Map of registered cooperations.
		 * 	std::map<std::string, so_5::coop_handle_t> live_coops_;
		 * 	...
		 * 	struct coop_is_alive final : public so_5::message_t
		 * 	{
		 * 		const std::string name_;
		 * 		const so_5::coop_handle_t handle_;
		 *
		 * 		coop_is_alive(std::string name, so_5::coop_handle_t handle)
		 * 			:	m_name{std::move(name)}
		 * 			,	m_handle{std::move(handle)}
		 * 		{}
		 * 	};
		 *
		 * 	void evt_coop_is_alive(mhood_t<coop_is_alive> cmd) {
		 * 		m_live_coops_[cmd->name_] = cmd->handle_;
		 * 	}
		 *
		 * 	void evt_make_new_coop(mhood_t<new_coop_info> cmd) {
		 * 		so_5::introduce_child_coop(*this, [&](so_5::coop_t & coop) {
		 * 			... // Fill the coop.
		 * 			// Add reg-notificator.
		 * 			coop.add_reg_notificator(
		 * 				[this, name=cmd->source_name](
		 * 					so_5::environment_t &,
		 * 					const so_5::coop_handle_t & handle) noexcept
		 * 				{
		 * 					// Inform the parent about the registration.
		 * 					so_5::send<coop_is_alive>(*this, name, handle);
		 * 				}
		 * 		} );
		 * 	}
		 *
		 * 	...
		 * }
		 * \endcode
		 *
		 * \note
		 * reg_notificator can (and most likely will) be called from some
		 * different thread, not the thread where reg_notificator was added. So
		 * please take an additional care if your notificator changes some shared
		 * data -- it can break thread safety easily.
		 *
		 * \attention
		 * Since v.5.6.0 reg_notificator should be a noexcept function or
		 * functor. Because of that a check in performed during compile time.  An
		 * attempt to pass non-noexcept function or functor to
		 * add_reg_notificator will lead to compilation error.
		 */
		template< typename Lambda >
		void
		add_reg_notificator(
			Lambda && notificator )
			{
				static_assert(
						std::is_nothrow_invocable_v<
								Lambda,
								environment_t &,
								const coop_handle_t & >,
						"notificator should be noexcept function/functor" );

				impl::coop_impl_t::add_reg_notificator(
						*this,
						coop_reg_notificator_t{
								std::forward<Lambda>(notificator)
						} );
			}

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Add notificator about cooperation deregistration event.
		 *
		 * Notificator should be a function or functional object with the
		 * following signature:
		 * \code
		 * void(
		 * 	so_5::environment_t & env,
		 * 	const so_5::coop_handle_t & handle,
		 * 	const so_5::coop_dereg_reason_t & reason) noexcept;
		 * \endcode
		 *
		 * An example:
		 * \code
		 * class parent_agent final : public so_5::agent_t
		 * {
		 * 	// Map of registered cooperations.
		 * 	std::map<std::string, so_5::coop_handle_t> live_coops_;
		 * 	...
		 * 	struct coop_is_dead final : public so_5::message_t
		 * 	{
		 * 		const std::string name_;
		 *
		 * 		explicit coop_is_dead(std::string name)
		 * 			:	m_name{std::move(name)}
		 * 		{}
		 * 	};
		 *
		 * 	void evt_coop_is_dead(mhood_t<coop_is_dead> cmd) {
		 * 		m_live_coops_.erase(cmd->name_);
		 * 	}
		 *
		 * 	void evt_make_new_coop(mhood_t<new_coop_info> cmd) {
		 * 		so_5::introduce_child_coop(*this, [&](so_5::coop_t & coop) {
		 * 			... // Fill the coop.
		 *
		 * 			// Add reg-notificator.
		 * 			coop.add_reg_notificator(...);
		 *
		 * 			// Add deref-notificator.
		 * 			coop.add_dereg_notificator(
		 * 				[this, name=cmd->source_name](
		 * 					so_5::environment_t &,
		 * 					const so_5::coop_handle_t & handle,
		 * 					const so_5::coop_dereg_reason_t &) noexcept
		 * 				{
		 * 					// Inform the parent about the deregistration.
		 * 					so_5::send<coop_is_dead>(*this, name);
		 * 				}
		 * 		} );
		 * 	}
		 *
		 * 	...
		 * }
		 * \endcode
		 *
		 * \note
		 * dereg_notificator can (and most likely will) be called from some
		 * different thread, not the thread where dereg_notificator was added. So
		 * please take an additional care if your notificator changes some shared
		 * data -- it can break thread safety easily.
		 *
		 * \attention
		 * Since v.5.6.0 dereg_notificator should be a noexcept function or
		 * functor. Because of that a check in performed during compile time. An
		 * attempt to pass non-noexcept function or functor to
		 * add_dereg_notificator will lead to compilation error.
		 */
		template< typename Lambda >
		void
		add_dereg_notificator(
			Lambda && notificator )
			{
				static_assert(
						std::is_nothrow_invocable_v<
								Lambda,
								environment_t &,
								const coop_handle_t &,
								const coop_dereg_reason_t & >,
						"notificator should be noexcept function/functor" );

				impl::coop_impl_t::add_dereg_notificator(
						*this,
						coop_dereg_notificator_t{
								std::forward<Lambda>(notificator)
						} );
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

				m_resource_deleters.emplace_back( resource_deleter_t{ ret_value } );

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
		[[nodiscard]]
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
		 auto coop = env.make_coop();
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
		 auto disp = so_5::disp::one_thread::make_dispatcher( env );
		 auto coop = env.make_coop();
		 // For the case of constructor like my_agent(environmen_t&).
		 coop->make_agent_with_binder< my_agent >( disp.binder() ); 
		 // For the case of constructor like your_agent(environment_t&, std::string).
		 auto ya = coop->make_agent_with_binder< your_agent >( disp.binder(), "hello" );
		 // For the case of constructor like thier_agent(environment_t&, std::string, mbox_t).
		 coop->make_agent_with_binder< their_agent >( disp.binder(), "bye", ya->so_direct_mbox() );
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
		
		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Deregister the cooperation with the specified reason.
		 *
		 * \par Usage example:
			\code
			coop_t & coop = ...;
			coop.deregister( so_4::dereg_reason::user_defined_reason + 100 );
			\endcode
		 *
		 * \note
		 * This method is marked as noexcept because there is no way
		 * to recover if any exception is raised here.
		 *
		 */
		void
		deregister(
			//! Reason of cooperation deregistration.
			int reason ) noexcept
			{
				impl::coop_impl_t::do_deregistration_specific_actions(
						*this,
						coop_dereg_reason_t{ reason } );
			}

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Deregistr the cooperation normally.
		 *
		 * This method is just a shorthand for:
			\code
			so_5::coop_t & coop = ...;
			coop.deregister( so_5::dereg_reason::normal );
			\endcode
		 *
		 * \note
		 * This method is marked as noexcept because there is no way
		 * to recover if any exception is raised here.
		 */
		void
		deregister_normally() noexcept
			{
				this->deregister( dereg_reason::normal );
			}

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
			coop_deregistering,
			//! Deregistration of the coop is in the final stage.
			deregistration_in_final_stage
		};

		/*!
		 * \brief Type of user resource deleter.
		 *
		 * Note. Before v.5.6.0 object of type std::function was used
		 * as resource deleter. Since v.5.6.0 with functor is used as
		 * lightweigt std::function alternative.
		 */
		struct resource_deleter_t
			{
				using deleter_pfn_t = void(*)(void *) noexcept;

				void * m_resource;
				deleter_pfn_t m_deleter;

				template< typename T >
				resource_deleter_t( T * resource )
					:	m_resource{ resource }
					,	m_deleter{ [](void * raw_ptr) noexcept {
								auto r = reinterpret_cast<T *>(raw_ptr);
								delete r;
							}
						}
					{}

				void
				operator()() noexcept
					{
						(*m_deleter)( m_resource );
					}
			};

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

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Deregistration reason.
		 *
		 * Receives actual value only in deregister() method.
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
		 * \since v.5.6.0
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
		 * \since v.5.6.0
		 */
		coop_shptr_t m_next_sibling;

		//FIXME: add more description to the comment.
		/*!
		 * \brief The next coop in the chain for final deregistration actions.
		 *
		 * \since v.5.8.0
		 */
		coop_shptr_t m_next_in_final_dereg_chain;

		/*!
		 * \brief Increment usage count for the coop.
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
		increment_usage_count() noexcept
			{
				++m_reference_count;
			}

		/*!
		 * \brief Decrement usage count for the coop.
		 *
		 * Note that is usage count is become 0 then final deregistration
		 * actions will be initiated.
		 *
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
		 *
		 * \note
		 * This method is marked as noexcept because there is no way
		 * to recover if any exception is raised here.
		 */
		void
		decrement_usage_count() noexcept
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

//FIXME: should this method be noexcept?
		/*!
		 * \brief Remove a child from the parent coop.
		 *
		 * This method is called by child coop.
		 *
		 * \note
		 * This method locks the parent coop object. But under that lock
		 * fields m_prev_sibling and m_next_sibling of the child coop
		 * are modified.
		 *
		 * \note
		 * In contradiction to add_child() this method receives a reference
		 * to child coop. This allows to avoid construction of temporary
		 * shared_ptr object.
		 *
		 * \since
		 * v.5.6.0
		 */
		void
		remove_child(
			//! Child coop to be removed.
			coop_t & child )
			{
				impl::coop_impl_t::do_remove_child( *this, child );
			}

		/*!
		 * \brief A helper method for doing some actions with children coops.
		 *
		 * This method is intended to be used in derived classes where an
		 * access to m_first_child, m_prev_sibling and m_next_sibling from
		 * another coop object will be prohibited.
		 *
		 * \since
		 * v.5.6.0
		 */
		template< typename Lambda >
		void
		for_each_child( Lambda && lambda ) const
			{
				auto * child = m_first_child.get();
				while( child )
				{
					lambda( *child );

					child = child->m_next_sibling.get();
				}
			}
	};

//
// coop_unique_holder_t
//
/*!
 * \brief A special type that plays role of unique_ptr for coop.
 *
 * In previous versions of SObjectizer std::unique_ptr was used for
 * holding coop before the coop will be passed to register_coop.
 * But in v.5.6 shared_ptr should be used for holding a pointer
 * to a coop object (otherwise a call to coop_t::shared_from_this()
 * can throw std::bad_weak_ptr in some cases).
 *
 * Class coop_unique_holder_t is an replacement for coop_unique_ptr_t
 * from previous versions of SObjectizer, but it holds shared_ptr inside.
 *
 * \since
 * v.5.6.0
 */
class coop_unique_holder_t
	{
		friend class impl::coop_private_iface_t;

		//! A pointer to coop object.
		coop_shptr_t m_coop;

		coop_shptr_t
		release() noexcept
			{
				return std::move(m_coop);
			}

	public :
		coop_unique_holder_t() = default;
		coop_unique_holder_t( coop_shptr_t coop ) : m_coop{ std::move(coop) } {}

		coop_unique_holder_t( const coop_unique_holder_t & ) = delete;
		coop_unique_holder_t( coop_unique_holder_t && ) = default;

		coop_unique_holder_t &
		operator=( const coop_unique_holder_t & ) = delete;

		coop_unique_holder_t &
		operator=( coop_unique_holder_t && ) noexcept = default;

		friend void
		swap( coop_unique_holder_t & a, coop_unique_holder_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_coop, b.m_coop );
			}

		operator bool() const noexcept { return static_cast<bool>(m_coop); }

		bool operator!() const noexcept { return !m_coop; }

		coop_t *
		get() const noexcept { return m_coop.get(); }

		coop_t *
		operator->() const noexcept { return m_coop.get(); }

		coop_t &
		operator*() const noexcept { return *m_coop.get(); }
	};


} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

