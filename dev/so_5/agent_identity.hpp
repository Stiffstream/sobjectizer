/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Types related to agents identity (name).
 */

#pragma once

#include <so_5/declspec.hpp>

#include <so_5/fwd.hpp>

#include <array>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

namespace so_5
{

/*!
 * \brief Helper class for holding agent's identity (name or pointer).
 *
 * Since v.5.8.2 agents may have an optional name. If a name is set for agent
 * a reference to this name can be returned by agent_t::so_agent_name(). But
 * if a name is not specified then only a pointer to agent (agent_t's `this` value)
 * can be used as agent's identity.
 *
 * This type is intended to be used as a thin wrapper that covers cases described
 * above. It holds a reference to agent's name if the name is specified or
 * just pointer to an agent if there is no name. This type can be seen as
 * a more convenient version of:
 * \code
 * std::variant<std::string_view, const agent_t*>;
 * \endcode
 *
 * \attention
 * Objects of this type hold references, not values. So it's not safe to store
 * them for a long time. Please consider to store `std::string` objects
 * returned by to_string() methods instead of agent_identity_t objects.
 *
 * \since v.5.8.2
 */
class agent_identity_t
	{
		friend class agent_t;

		//! Type for case when agent has no user-provided name.
		struct pointer_only_t
			{
				//! Capacity of c-string for holding string representation.
				//!
				//! Size of hex representation of a pointer + "<noname:>".length() + 1.
				static constexpr std::size_t c_string_size =
						(sizeof(void*) * 2u)
						+ std::string_view{ "<noname:>" }.size()
						+ 1u /* terminating 0-symbol */;

				//! Value.
				const agent_t * m_pointer_value;

				// NOTE: this method is implemented in agent.cpp source file.
				//! Make a c-string with text representation of a value.
				[[nodiscard]] SO_5_FUNC
				std::array<char, c_string_size>
				make_c_string() const noexcept;
			};

		//! Type for case when agent has user-provided name.
		struct actual_name_t
			{
				//! Value.
				std::string_view m_name;
			};

		/*!
		 * Helper type to be used with std::visit when agent_identity has to
		 * be converted into a string.
		 */
		struct to_string_visitor_t
			{
				[[nodiscard]]
				std::string
				operator()( const pointer_only_t & v ) const
					{
						const auto c_str = v.make_c_string();
						return std::string( c_str.data(), pointer_only_t::c_string_size - 1u );
					}

				[[nodiscard]]
				std::string
				operator()( const actual_name_t & v ) const
					{
						return std::string( v.m_name );
					}
			};

		/*!
		 * Helper type to be used with std::visit when agent_identity has to
		 * be printed into a std::ostream.
		 */
		struct to_ostream_visitor_t
			{
				std::ostream & m_to;

				to_ostream_visitor_t( std::ostream & to ) : m_to{ to } {}

				void
				operator()( const pointer_only_t & v ) const
					{
						const auto c_str = v.make_c_string();
						m_to << c_str.data();
					}

				void
				operator()( const actual_name_t & v ) const
					{
						m_to << v.m_name;
					}
			};

		//! Type of identity holder.
		using value_t = std::variant< pointer_only_t, actual_name_t >;

		//! Agent's identity.
		value_t m_value;

		//! Initializing constructor for case when agent has no user specified name.
		agent_identity_t( const agent_t * pointer ) noexcept
			:	m_value{ pointer_only_t{ pointer } }
			{}

		//! Initializing constructor for case when agent has a user specified name.
		agent_identity_t( std::string_view name ) noexcept
			:	m_value{ actual_name_t{ name } }
			{}

	public:
		//! Does agent have an actual name?
		[[nodiscard]]
		bool
		has_actual_name() const noexcept
			{
				return std::holds_alternative< actual_name_t >( m_value );
			}

		//! Attempt to get an agent name.
		/*!
		 * \return empty std::string_view if agent has no an actual name.
		 */
		[[nodiscard]]
		std::string_view
		actual_name() const noexcept
			{
				if( const auto * an = std::get_if< actual_name_t >( &m_value ) )
					return an->m_name;
				else
					return std::string_view{};
			}

		//! Transform identity into a string.
		[[nodiscard]]
		std::string
		to_string() const
			{
				return std::visit( to_string_visitor_t{}, m_value );
			}

		friend std::ostream &
		operator<<( std::ostream & to, const agent_identity_t & what )
			{
				std::visit( to_ostream_visitor_t{ to }, what.m_value );
				return to;
			}
	};

} /* namespace so_5 */

