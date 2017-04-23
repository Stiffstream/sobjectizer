/*
 * SObjectizert-5
 */

/*!
 * \file
 * \brief Basic tools for simplify usage of std::mutex or null_mutex
 *
 * \since
 * v.5.5.19
 */

#pragma once

#include <mutex>

namespace so_5 {

namespace details {

//
// actual_lock_holder_t
//
/*!
 * \brief A class to be used as mixin with actual std::mutex instance inside.
 *
 * Usage example:
 * \code
   template< typename LOCK_HOLDER >
	class coop_repo_t final : protected LOCK_HOLDER
	{
	public :
		bool has_live_coop()
			{
				this->lock_and_perform([&]{ return !m_coops.empty(); });
			}
	};

	using mtsafe_coop_repo_t = coop_repo_t< so_5::details::actual_lock_holder_t >;
 * \endcode
 *
 * \since
 * v.5.5.19
 */
class actual_lock_holder_t
	{
		//! Actual lock.
		/*!
		 * \note
		 * This is mutable attibute because locking can be necessary
		 * even in const methods of derived classes.
		 */
		mutable std::mutex m_lock;

	public :
		//! Do actual lock and perform necessary action.
		template< typename LAMBDA >
		auto
		lock_and_perform( LAMBDA && l ) const -> decltype(l())
			{
				std::lock_guard< std::mutex > lock{ m_lock };
				return l();
			}
	};

//
// no_lock_holder_t
//
/*!
 * \brief A class to be used as mixin without any real mutex instance inside.
 *
 * Usage example:
 * \code
   template< typename LOCK_HOLDER >
	class coop_repo_t final : protected LOCK_HOLDER
	{
	public :
		bool has_live_coop()
			{
				this->lock_and_perform([&]{ return !m_coops.empty(); });
			}
	};

	using non_mtsafe_coop_repo_t = coop_repo_t< so_5::details::no_lock_holder_t >;
 * \endcode
 *
 * \since
 * v.5.5.19
 */
class no_lock_holder_t
	{
	public :
		//! Perform necessary action.
		template< typename LAMBDA >
		auto
		lock_and_perform( LAMBDA && l ) const -> decltype(l())
			{
				return l();
			}
	};

} /* namespace details */

} /* namespace so_5 */

