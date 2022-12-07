/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.3
 *
 * \file
 * \brief A collector for agent tuning options.
 */

#pragma once

#include <so_5/subscription_storage_fwd.hpp>
#include <so_5/message_limit.hpp>
#include <so_5/priority.hpp>

namespace so_5
{

/*!
 * \brief Wrapper around a pointer to partially constructed agent.
 *
 * This type is used as indicator that a pointer to agent can be
 * transferred or stored somewhere, but shouldn't be used because
 * the agent isn't fully constructed yet.
 *
 * \since v.5.7.4
 */
class partially_constructed_agent_ptr_t
	{
		agent_t * m_ptr;

	public:
		explicit partially_constructed_agent_ptr_t( agent_t * ptr )
			:	m_ptr{ ptr }
			{}

		[[nodiscard]]
		agent_t *
		ptr() const noexcept { return m_ptr; }
	};

/*!
 * \brief Type of functor to be used as a factory for custom direct mbox.
 *
 * \attention
 * The factory should return a valid mbox or should throw an exception.
 * The behavior isn't defined if the factory return empty so_5::mbox_t
 * object (the application may crash in the better case).
 *
 * \since v.5.7.4
 */
using custom_direct_mbox_factory_t = std::function<
		so_5::mbox_t(partially_constructed_agent_ptr_t, so_5::mbox_t) >;

//
// agent_tuning_options_t
//
/*!
 * \since
 * v.5.5.3
 *
 * \brief A collector for agent tuning options.
 */
class agent_tuning_options_t
	{
	public :
		agent_tuning_options_t() = default;

		friend inline void
		swap(
			so_5::agent_tuning_options_t & a,
			so_5::agent_tuning_options_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_subscription_storage_factory,
						b.m_subscription_storage_factory );
				swap( a.m_message_limits, b.m_message_limits );
				swap( a.m_priority, b.m_priority );
				swap( a.m_custom_direct_mbox_factory,
						b.m_custom_direct_mbox_factory );
			}

		//! Set factory for subscription storage creation.
		agent_tuning_options_t &
		subscription_storage_factory(
			subscription_storage_factory_t factory )
			{
				m_subscription_storage_factory = std::move( factory );

				return *this;
			}

		[[nodiscard]]
		const subscription_storage_factory_t &
		query_subscription_storage_factory() const noexcept
			{
				return m_subscription_storage_factory;
			}

		//! Default subscription storage factory.
		static subscription_storage_factory_t
		default_subscription_storage_factory()
			{
				return so_5::default_subscription_storage_factory();
			}

		[[nodiscard]]
		message_limit::description_container_t
		giveout_message_limits()
			{
				return std::move( m_message_limits );
			}

		template< typename... Args >
		agent_tuning_options_t &
		message_limits( Args &&... args )
			{
				message_limit::accept_indicators(
						m_message_limits,
						std::forward< Args >( args )... );

				return *this;
			}

		//! Set priority for agent.
		/*! \since
		 * v.5.5.8
		 */
		agent_tuning_options_t &
		priority( so_5::priority_t v )
			{
				m_priority = v;
				return *this;
			}

		//FIXME: this method should be marked as [[nodiscard]] and
		//noexcept in SO-5.8.
		//! Get priority value.
		so_5::priority_t
		query_priority() const
			{
				return m_priority;
			}

		/*!
		 * \brief Set custom direct mbox factory.
		 *
		 * \since v.5.7.4
		 */
		agent_tuning_options_t &
		custom_direct_mbox_factory(
			custom_direct_mbox_factory_t factory )
			{
				m_custom_direct_mbox_factory = factory;

				return *this;
			}

		/*!
		 * \brief Get a reference to custom direct mbox factory.
		 *
		 * \note
		 * If the factory isn't set then a reference to empty
		 * std::function object is returned.
		 *
		 * \since v.5.7.4
		 */
		[[nodiscard]]
		const custom_direct_mbox_factory_t &
		query_custom_direct_mbox_factory() const noexcept
			{
				return m_custom_direct_mbox_factory;
			}

	private :
		subscription_storage_factory_t m_subscription_storage_factory =
				default_subscription_storage_factory();

		message_limit::description_container_t m_message_limits;

		//! Priority for agent.
		/*!
		 * \since v.5.5.8
		 */
		so_5::priority_t m_priority = so_5::prio::default_priority;

		/*!
		 * \brief Optional factory for custom direct mboxes.
		 *
		 * It can be an empty std::function object.
		 *
		 * \since v.5.7.4
		 */
		custom_direct_mbox_factory_t m_custom_direct_mbox_factory;
	};

} /* namespace so_5 */

