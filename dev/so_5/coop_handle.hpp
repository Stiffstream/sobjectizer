/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Type of smart handle to coop.
 * 
 * \since
 * v.5.6.0
 */

#pragma once

#include <so_5/compiler_features.hpp>
#include <so_5/exception.hpp>
#include <so_5/types.hpp>

#include <memory>
#include <string>
#include <iostream>

namespace so_5
{

class coop_t;

//! Typedef for the agent_coop smart pointer.
using coop_shptr_t = std::shared_ptr< coop_t >;

//
// coop_handle_t
//
//FIXME: more documentation is needed.
/*!
 * \brief Type of smart handle for a cooperation.
 *
 * \since
 * v.5.6.0
 */
class SO_5_NODISCARD coop_handle_t
	{
		friend class so_5::coop_t;

		//! ID of cooperation.
		coop_id_t m_id;

		//! Pointer for cooperation.
		/*!
		 * \attention
		 * This is a weak pointer. It means that it can refer to already
		 * destroyed cooperation.
		 */
		std::weak_ptr< coop_t > m_coop;

		//! Initializing constructor.
		coop_handle_t( coop_id_t id, std::shared_ptr< coop_t > coop )
			:	m_id{ id }, m_coop{ coop }
			{}

	public :
		static constexpr coop_id_t invalid_coop_id = 0u;

		coop_handle_t() : m_id{ invalid_coop_id }
			{}

		operator bool() const noexcept { return invalid_coop_id != m_id; }

		bool operator!() const noexcept { return invalid_coop_id == m_id; }

		auto id() const noexcept { return m_id; }

//FIXME: should this method be public?
		coop_shptr_t
		to_shptr() const
			{
				auto result = m_coop.lock();
				if( !result )
					SO_5_THROW_EXCEPTION(
							rc_coop_already_destroyed,
							"coop object already destroyed, coop_id=" +
							std::to_string(m_id) );
				return result;
			}

//FIXME: should this method be public?
		coop_shptr_t
		to_shptr_noexcept() const noexcept
			{
				return m_coop.lock();
			}

		//! A tool for dumping coop_handle to ostream.
		friend inline std::ostream &
		operator<<( std::ostream & to, const coop_handle_t & what )
		{
			if( what )
				return (to << "{coop:id=" << what.m_id << "}");
			else
				return (to << "{empty-coop-handle}");
		}
	};

} /* namespace so_5 */

