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

#include <memory>

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

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

// NOTE: the implementation of name_for_agent_t is in agent.cpp

/*!
 * \brief Type for holding agent name.
 *
 * Unlike std::string it has no small-string optimization, but its size is just
 * a (sizeof(char *)+sizeof(unsigned int)), so it is smaller than std::string in
 * cases when agent's name is empty.
 *
 * \note
 * This type holds a copy of name (without the terminal 0-symbol). If name is
 * not set (or if the content of the object is moved out) then a nullptr is
 * held and the object is treated as empty or null-object.
 *
 * \attention
 * The size of a name has to be fit into `unsigned int`, not into `std::size_t`,
 * so name of agent can't exceed 4GiB if `unsigned int` is 32-bit.
 *
 * \since v.5.8.2
 */
class SO_5_TYPE name_for_agent_t
{
	static_assert( sizeof(unsigned int) <= sizeof(std::size_t),
			"it's expected that std::size_t is bigger or the same as unsigned int" );

	//! Name storage.
	/*!
	 * \note
	 * It doesn't have a terminating 0-symbol.
	 */
	std::unique_ptr< char[] > m_value;

	//! Name length.
	unsigned int m_length;

public:
	//! Default constructor makes an null value.
	name_for_agent_t();

	//! Initializing constructor.
	/*!
	 * \attention
	 * It throws is \a value has zero length.
	 *
	 * \attention
	 * It throws is the length of \a value can't fit into unsigned int.
	 */
	explicit name_for_agent_t( std::string_view value );

	// Copy operation.
	name_for_agent_t( const name_for_agent_t & );
	name_for_agent_t &
	operator=( const name_for_agent_t & );

	// Move operation.
	name_for_agent_t( name_for_agent_t && other ) noexcept;
	name_for_agent_t &
	operator=( name_for_agent_t && other ) noexcept;

	~name_for_agent_t();

	friend SO_5_FUNC void swap( name_for_agent_t & a, name_for_agent_t & b ) noexcept;

	//! Get the value as a string_view.
	/*!
	 * \note
	 * If the object is null, then it produces an empty string_view.
	 */
	[[nodiscard]]
	std::string_view
	as_string_view() const;

	//! Does this object have a value?
	[[nodiscard]]
	bool
	has_value() const noexcept;

	//! Does this object have a value?
	explicit operator bool() const noexcept
		{
			return this->has_value();
		}
};

/*!
 * \brief Helper type with method to be mixed into agent class.
 *
 * \since v.5.8.2
 */
struct name_for_agent_methods_mixin_t
{
	/*!
	 * \brief A helper factory for making #name_for_agent_t instance.
	 *
	 * Usage example:
	 * \code
	 * class my_agent final : public so_5::agent_t
	 * {
	 * public:
	 * 	my_agent(context_t ctx, std::string_view name)
	 * 		: so_5::agent_t{ ctx + name_for_agent(name) }
	 * 	{}
	 * 	...
	 * }
	 * \endcode
	 *
	 * \since v.5.8.2
	 */
	[[nodiscard]]
	static name_for_agent_t
	name_for_agent( std::string_view name )
		{
			return name_for_agent_t{ name };
		}
};

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

//
// agent_tuning_options_t
//
/*!
 * \brief A collector for agent tuning options.
 *
 * \attention
 * This class isn't thread-safe.
 *
 * \since v.5.5.3
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
				swap( a.m_is_user_provided_subscription_storage_factory,
						b.m_is_user_provided_subscription_storage_factory );
				swap( a.m_agent_name, b.m_agent_name );
			}

		//! Set factory for subscription storage creation.
		agent_tuning_options_t &
		subscription_storage_factory(
			subscription_storage_factory_t factory )
			noexcept( noexcept(std::declval<subscription_storage_factory_t &>() = std::move(factory)) )
			{
				m_subscription_storage_factory = std::move( factory );
				m_is_user_provided_subscription_storage_factory = true;

				return *this;
			}

		//FIXME(v.5.9.0): this method has to be changed to this one:
		//
		// std::optional<subscription_storage_factory_t>
		// query_subscription_storage_factory() const.
		//
		[[nodiscard]]
		const subscription_storage_factory_t &
		query_subscription_storage_factory() const noexcept
			{
				return m_subscription_storage_factory;
			}

		/*!
		 * \brief Does a user provide a specific subscription_storage_factory?
		 *
		 * \retval false If subscription_storage_factory wasn't specified by a user.
		 * \retval true If the subscription_storage_factory was specified by a user.
		 *
		 * \since v.5.8.2
		 */
		[[nodiscard]]
		bool
		is_user_provided_subscription_storage_factory() const noexcept
			{
				return m_is_user_provided_subscription_storage_factory;
			}

		//FIXME(v.5.9.0): should this method be removed?
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
		/*!
		 * \since v.5.5.8
		 */
		agent_tuning_options_t &
		priority( so_5::priority_t v )
			{
				m_priority = v;
				return *this;
			}

		//! Get priority value.
		[[nodiscard]]
		so_5::priority_t
		query_priority() const noexcept
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

		/*!
		 * \brief Set a name for agent.
		 *
		 * \note
		 * The name should have an actual name, otherwise an exception
		 * will be thrown.
		 *
		 * \since v.5.8.2
		 */
		agent_tuning_options_t &
		agent_name( name_for_agent_t name )
			{
				if( !name.has_value() )
					SO_5_THROW_EXCEPTION( rc_empty_agent_name,
							"empty name can't be used for an agent" );

				m_agent_name = std::move(name);

				return *this;
			}

		/*!
		 * \brief Does a name specified for an agent?
		 *
		 * \since v.5.8.2
		 */
		[[nodiscard]]
		bool
		has_agent_name() const noexcept
			{
				return m_agent_name.has_value();
			}

		/*!
		 * \brief Gives away the name for an agent.
		 *
		 * If the name wasn't set an empty value is returned.
		 *
		 * \since v.5.8.2
		 */
		[[nodiscard]]
		name_for_agent_t
		giveout_agent_name() noexcept
			{
				return name_for_agent_t{ std::move(m_agent_name) };
			}

	private :
		//FIXME(v.5.9.0): this member has to be changed to:
		//
		// std::optional<subscription_storage_factory_t> m_subscription_storage_factory;
		//
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

		//FIXME(v.5.9.0): this member has to be removed.
		/*!
		 * \brief Does a user provide a specific subscription_storage_factory?
		 *
		 * This member is set to `true` in subscription_storage_factory() setter.
		 *
		 * \since v.5.8.2
		 */
		bool m_is_user_provided_subscription_storage_factory{ false };

		/*!
		 * \brief Optional name for an agent.
		 *
		 * \since v.5.8.2
		 */
		name_for_agent_t m_agent_name;
	};

} /* namespace so_5 */

