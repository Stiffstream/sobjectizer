/*
 * SObjectizer-5
 */

#pragma once

#include <cstring>
#include <sstream>
#include <stdexcept>

inline bool
is_arg(
	const char * cmd_line_arg,
	const char * v1,
	const char * v2 )
	{
		return (0 == std::strcmp( cmd_line_arg, v1 ) ||
				0 == std::strcmp( cmd_line_arg, v2 ));
	}

template< class T >
void
arg_to_value( T & receiver,
	const char * cmd_line_arg_value,
	const char * name,
	const char * description )
	{
		std::stringstream ss;
		ss << cmd_line_arg_value;
		ss.seekg(0);

		T r;
		ss >> r;

		if( !ss || !ss.eof() )
			throw std::runtime_error(
					std::string( "unable to parse value for argument '" ) + name +
					"' (" + description + "): " +
					cmd_line_arg_value );

		receiver = r;
	}

template< class T >
void
mandatory_arg_to_value(
	T & receiver,
	char ** arg, char ** last_arg,
	const char * name,
	const char * description )
	{
		if( arg == last_arg )
			throw std::runtime_error(
					std::string( "argument '" ) + name + "' requires value (" +
					description + ")" );

		return arg_to_value( receiver, *arg, name, description );
	}

