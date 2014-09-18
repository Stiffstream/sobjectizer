/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief A workaround for very slow implementation of
 * std::this_thread::get_id() under some compilers.
 */

#pragma once

#include <so_5/h/compiler_features.hpp>

#if !defined( SO_5_MSVC_NEEDS_OWN_CURRENT_THREAD_ID )
// For the normal implementations use the standard tools.

#include <thread>

namespace so_5
{
	//! Type of the current thread id.
	typedef std::thread::id current_thread_id_t;

	//! Get the ID of the current thread.
	inline current_thread_id_t
	query_current_thread_id()
		{
			return std::this_thread::get_id();
		}

	//! Get NULL thread id.
	inline current_thread_id_t
	null_current_thread_id()
		{
			return std::thread::id();
		}

} /* namespace so_5 */

#else

// For Visual C++ we need to define our own id facilities.

#include <iostream>
#include <limits>
#include <cstdint>

#include <so_5/h/declspec.hpp>

namespace so_5
{
	//! A special wrapper around native thread ID.
	class current_thread_id_t
		{
			friend SO_5_EXPORT_FUNC_SPEC(current_thread_id_t) query_current_thread_id();

		public :
			typedef std::uint_least64_t id_t;

			static const id_t invalid_id = static_cast< id_t >( -1 );

			//! Constructs invalid_id.
			inline current_thread_id_t()
				:	m_id( invalid_id )
				{}

			inline bool
			operator==( const current_thread_id_t & o ) const
				{
					return m_id == o.m_id;
				}

			inline bool
			operator!=( const current_thread_id_t & o ) const
				{
					return m_id != o.m_id;
				}

			inline bool
			operator<( const current_thread_id_t & o ) const
				{
					return m_id < o.m_id;
				}

			inline bool
			operator<=( const current_thread_id_t & o ) const
				{
					return m_id <= o.m_id;
				}

			inline id_t
			id() const
				{
					return m_id;
				}

		private :
			//! Actual constructor.
			inline current_thread_id_t( id_t id )
				:	m_id( id )
				{}

			id_t m_id;
		};

	//! Get NULL thread id.
	inline current_thread_id_t
	null_current_thread_id()
		{
			return current_thread_id_t();
		}

	inline std::ostream &
	operator<<( std::ostream & o, const current_thread_id_t & id )
		{
			if( id == null_current_thread_id() )
				return (o << "<thread_id:NULL>");

			return (o << "<thread_id:" << id.id() << ">");
		}

	//! Get the ID of the current thread.
	SO_5_EXPORT_FUNC_SPEC( current_thread_id_t )
	query_current_thread_id();

} /* namespace so_5 */

#endif

