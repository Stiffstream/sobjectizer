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
template< typename T, typename... ARGS >
std::unique_ptr< T >
make_unique( ARGS && ...args )
	{
		return std::unique_ptr< T >( new T( std::forward<ARGS>(args)... ) );
	}

} /* namespace stdcpp */

} /* namespace so_5 */

