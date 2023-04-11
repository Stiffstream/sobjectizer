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
#include <string_view>

#include <so_5/declspec.hpp>
#include <so_5/compiler_features.hpp>
#include <so_5/ret_code.hpp>

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
	public:
		exception_t(
			const std::string & error_descr,
			int error_code )
			:	std::runtime_error( error_descr )
			,	m_error_code( error_code )
			{}
		exception_t( const exception_t & ) = default;
		exception_t( exception_t && ) = default;

		exception_t &
		operator=( exception_t & o ) = default;

		exception_t &
		operator=( exception_t && o ) noexcept = default;

		//! Error code getter.
		int
		error_code() const noexcept { return m_error_code; }

		static void
		raise(
			const char * file_name,
			unsigned int line_number,
			std::string_view error_descr,
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

