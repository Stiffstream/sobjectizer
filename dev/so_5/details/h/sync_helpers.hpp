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

//
// null_mutex_t
//
/*!
 * \brief A class which is like std::mutex but does not do any real actions.
 *
 * \since
 * v.5.5.19.2
 */
class null_mutex_t
	{
	public :
		null_mutex_t() {}
		null_mutex_t( const null_mutex_t & ) = delete;
		null_mutex_t( null_mutex_t && ) = delete;

		void lock() {}
		void unlock() {}
	};

namespace details {

//
// actual_lock_holder_t
//
/*!
 * \brief A class to be used as mixin with actual std::mutex instance inside.
 *
 * Usage example:
 * \code
   template< typename Lock_Holder >
	class coop_repo_t final : protected Lock_Holder
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
 * \tparam Lock_Type type of lock to be used for object protection.
 * Will be used with std::lock_guard.
 *
 * \since
 * v.5.5.19
 */
template< typename Lock_Type = std::mutex >
class actual_lock_holder_t
	{
		//! Actual lock.
		/*!
		 * \note
		 * This is mutable attibute because locking can be necessary
		 * even in const methods of derived classes.
		 */
		mutable Lock_Type m_lock;

	public :
		//! Do actual lock and perform necessary action.
		template< typename Lambda >
		auto
		lock_and_perform( Lambda && l ) const -> decltype(l())
			{
				std::lock_guard< Lock_Type > lock{ m_lock };
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
   template< typename Lock_Holder >
	class coop_repo_t final : protected Lock_Holder
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
		template< typename Lambda >
		auto
		lock_and_perform( Lambda && l ) const -> decltype(l())
			{
				return l();
			}
	};

//
// lock_holder_detector
//
/*!
 * \brief A selector of actual lock_holder type in dependency of lock type.
 *
 * Usage example:
 * \code
 * template< typename Lock_Type >
 * class my_thread_safe_class
 * 	: public so_5::details::lock_holder_detector<Lock_Type>::type
 * 	{
 * 		void some_method() {
 * 			this->lock_and_perform( [&] {
 * 				... // Some actions.
 * 			} );
 * 		}
 * 	};
 * \endcode
 *
 * \since
 * v.5.5.19.2
 */
template< typename Lock_Type >
struct lock_holder_detector
	{
		using type = actual_lock_holder_t<Lock_Type>;
	};

template<>
struct lock_holder_detector<null_mutex_t>
	{
		using type = no_lock_holder_t;
	};

} /* namespace details */

} /* namespace so_5 */

