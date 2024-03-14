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

//FIXME: document this!
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
				const void * m_pointer_value;

				// NOTE: this method is implemented in agent.cpp source file.
				//! Make a c-string with text representation of a value.
				SO_5_FUNC [[nodiscard]]
				std::array<char, c_string_size>
				make_c_string() const noexcept;
			};

		//! Type for case when agent has user-provided name.
		struct actual_name_t
			{
				//! Value.
				std::string_view m_name;
			};

		//FIXME: document this!
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

		//FIXME: document this!
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
		agent_identity_t( const void * pointer ) noexcept
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

