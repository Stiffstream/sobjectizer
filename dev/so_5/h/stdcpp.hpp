/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.18
 *
 * \brief Various standard C++ stuff from newer versions of C++
 * which we want to use with older C++ compilers.
 */

#pragma once

#include <utility>
#include <memory>

namespace so_5 {

namespace stdcpp {

//
// make_unique
//
template< typename T, typename... Args >
std::unique_ptr< T >
make_unique( Args && ...args )
	{
		return std::unique_ptr< T >( new T( std::forward<Args>(args)... ) );
	}

} /* namespace stdcpp */

} /* namespace so_5 */

