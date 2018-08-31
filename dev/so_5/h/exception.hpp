/*
	SObjectizer 5.
*/

/*!
	\file
	\brief An exception class.
*/

#pragma once

#include <stdexcept>
#include <string>

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>
#include <so_5/h/ret_code.hpp>

namespace so_5
{

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4275)
#endif

//
// exception_t
//

//! The base class for all SObjectizer exceptions.
class SO_5_TYPE exception_t : public std::runtime_error
{
		//! Typedef for the base type.
		typedef std::runtime_error base_type_t;
	public:
		exception_t(
			const std::string & error_descr,
			int error_code )
			:	base_type_t( error_descr )
			,	m_error_code( error_code )
			{}
		exception_t( const exception_t & ) = default;
#if defined(SO_5_NEED_MSVC_12_WORKAROUNDS)
		exception_t( exception_t && o )
			:	base_type_t( std::move(o) )
			,	m_error_code( o.m_error_code )
			{}
#else
		exception_t( exception_t && ) = default;
#endif

		exception_t &
		operator=( exception_t & o ) = default;

#if defined(SO_5_NEED_MSVC_12_WORKAROUNDS)
		exception_t &
		operator=( exception_t && o )
			{
				base_type_t::operator=( std::move(o) );
				m_error_code = o.m_error_code;
				return *this;
			}
#else
		exception_t &
		operator=( exception_t && o ) = default;
#endif

		//! Error code getter.
		int
		error_code() const SO_5_NOEXCEPT { return m_error_code; }

		static void
		raise(
			const char * file_name,
			unsigned int line_number,
			const std::string & error_descr,
			int error_code );

	private:
		//! Error code.
		int m_error_code;
};

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

#define SO_5_THROW_EXCEPTION_IMPL(file, line, error_code, desc)\
	so_5::exception_t::raise(file, line, (desc), (error_code))

#define SO_5_THROW_EXCEPTION(error_code, desc)\
	SO_5_THROW_EXCEPTION_IMPL(__FILE__, __LINE__, error_code, desc)

} /* namespace so_5 */

