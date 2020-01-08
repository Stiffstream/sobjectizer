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

class coop_handle_t;

namespace low_level_api
{

coop_shptr_t
to_shptr( const coop_handle_t & );

coop_shptr_t
to_shptr_noexcept( const coop_handle_t & ) noexcept;

} /* namespace low_level_api */

//
// coop_handle_t
//
/*!
 * \brief Type of smart handle for a cooperation.
 *
 * Type coop_handle_t is used for references to registered coops.
 * Before v.5.6 coops in SObjectizer-5 were identified via strings.
 * Every coop had its own name.
 *
 * Since v.5.6.0 names are no more used for identificators of coops.
 * SObjectizer's environment_t returns instances of coop_handle_t for
 * every registered coop. This handle can be used for deregistration
 * of the coop at the appropriate time. For example:
 * \code
 * class request_manager_t final : public so_5::agent_t {
 * 	std::map<request_id_t, so_5::coop_handle_t> active_requests_;
 * 	...
 * 	void on_new_request(mhood_t<request_t> cmd) {
 * 		// Create a new coop for handling that request.
 * 		auto coop = so_5::create_child_coop( *this );
 * 		...; // Fill the coop with agents.
 * 		// Register coop and store its handle in the map of actual requests.
 * 		active_requests_[cmd->request_id()] = so_environment().register_coop(std::move(coop));
 * 	}
 * 	...
 * 	void on_cancel_request(mhood_t<cancel_request_t> cmd) {
 * 		auto it = active_requests_.find(cmd->request_id());
 * 		if(it != active_requests_.end())
 * 			// Deregister coop for this request.
 * 			so_environment().deregister_coop(it->second);
 * 	}
 * };
 * \endcode
 *
 * Note that coop_handle_t is somewhat like (smart)pointer. It can be empty,
 * e.g. do not point to any coop. Or it can be not-empty. In that case
 * coop_handle_t is very similar to std::weak_ptr.
 *
 * \since
 * v.5.6.0
 */
class coop_handle_t
	{
		friend class so_5::coop_t;

		friend coop_shptr_t
		low_level_api::to_shptr( const coop_handle_t & );

		friend coop_shptr_t
		low_level_api::to_shptr_noexcept( const coop_handle_t & ) noexcept;

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
		static constexpr const coop_id_t invalid_coop_id = 0u;

		coop_handle_t() : m_id{ invalid_coop_id }
			{}

		//! Is this handle empty?
		/*!
		 * Handle is empty if there is no underlying coop.
		 *
		 * \retval true if handle is not empty
		 */
		operator bool() const noexcept { return invalid_coop_id != m_id; }

		//! Is this non-empty handle?
		/*!
		 * Handle is empty if there is no underlying coop.
		 *
		 * \retval true if handle is empty
		 */
		bool operator!() const noexcept { return invalid_coop_id == m_id; }

		//! Get the ID of the coop.
		auto id() const noexcept { return m_id; }

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

namespace low_level_api
{

/*!
 * \brief A helper function for safe extraction of shared_ptr to
 * coop from coop_handle instance.
 *
 * \note
 * This function throws if coop object is already destroyed.
 *
 * \attention
 * This is a part of low-level SObjectizer's API.
 *
 * \since
 * v.5.6.0
 */
[[nodiscard]]
inline coop_shptr_t
to_shptr( const coop_handle_t & handle )
	{
		auto result = handle.m_coop.lock();
		if( !result )
			SO_5_THROW_EXCEPTION(
					rc_coop_already_destroyed,
					"coop object already destroyed, coop_id=" +
					std::to_string(handle.m_id) );
		return result;
	}

/*!
 * \brief A helper function for unsafe extraction of shared_ptr to
 * coop from coop_handle instance.
 *
 * \note
 * This function don't throws if coop object is already destroyed.
 * An empty shared_ptr is returned instead.
 *
 * \attention
 * This is a part of low-level SObjectizer's API.
 *
 * \since
 * v.5.6.0
 */
[[nodiscard]]
inline coop_shptr_t
to_shptr_noexcept( const coop_handle_t & handle ) noexcept
	{
		return handle.m_coop.lock();
	}

} /* namespace low_level_api */

} /* namespace so_5 */

