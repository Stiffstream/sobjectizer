/*
 * A very simple of library with plain C interface with usage
 * of SObjectizer inside.
 */

#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>

// Main SObjectizer header file.
#include <so_5/all.hpp>

//
// Library interface.
//
struct converter;
extern "C" int create_converter( converter ** handle_receiver );
extern "C" int convert_value(
	converter * handle, 
	const char * source_value,
	int * receiver );
extern "C" void destroy_converter( converter * handle );

//
// Library usage.
//

// Demo function which tries to convert all source values
// and returns vector of conversion descriptions results.
std::vector< std::string >
make_conversion(
	const std::vector< std::string > & values )
{
	// Helper for creation of converter instance.
	converter * handle = nullptr;

	const int rc1 = create_converter( &handle );
	if( rc1 )
		throw std::runtime_error( "converter creation error: " +
				std::to_string( rc1 ) );

	// Use unique_ptr to automatically delete converter at exit.
	std::unique_ptr< converter, void (*)(converter *) > converter_deleter(
			handle, destroy_converter );

	std::vector< std::string > result;

	// Do conversion and collecting of result descriptions.
	for( const auto & s : values )
	{
		int int_value = 0;
		const int rc2 = convert_value( handle, s.c_str(), &int_value );

		if( rc2 )
			result.push_back( "error=" + std::to_string( rc2 ) );
		else
			result.push_back( "success=" + std::to_string( int_value ) );
	}

	return result;
}

// Main demo loop.
void demo()
{
	// Two source sequences to be processed.
	std::vector< std::string > seq1;
	seq1.push_back( "1" );
	seq1.push_back( "2" );
	seq1.push_back( "three" );
	seq1.push_back( "4" );

	std::vector< std::string > seq2;
	seq2.push_back( "11" );
	seq2.push_back( "12" );
	seq2.push_back( "thirteen" );
	seq2.push_back( "14" );

	// Initiate asynchronous processing of sequences.
	auto f1 = std::async( make_conversion, std::cref(seq1) );
	auto f2 = std::async( make_conversion, std::cref(seq2) );

	// Collecting and printing results.
	auto printer = [](
		const char * name,
		const std::vector< std::string > & result ) {
			std::cout << name << ": " << std::flush;
			for( const auto & i : result )
				std::cout << i << ",";
			std::cout << std::endl;
		};

	printer( "First sequence", f1.get() );
	printer( "Second sequence", f2.get() );
}

int main()
{
	try
	{
		demo();
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
		return 2;
	}

	return 0;
}

//
// Library implementation.
//

extern "C" int create_converter( converter ** handle_receiver )
{
	try
	{
		std::unique_ptr< so_5::wrapped_env_t > env{ new so_5::wrapped_env_t{} };

		// A single coop with conversion agent must be added into Environment.
		env->environment().introduce_coop( [&]( so_5::coop_t & coop ) {
			// Mbox for conversion messages.
			auto mbox = coop.environment().create_mbox( "converter" );
			// Converter agent.
			coop.define_agent().event( mbox, []( const std::string & v ) -> int {
					std::istringstream s{ v };
					int result;
					s >> result;
					if( s.fail() )
						throw std::invalid_argument( "unable to convert to int: '" + v + "'" );
					return result;
				} );
			} );

		// Result handle for converter Environment.
		*handle_receiver = reinterpret_cast< converter * >( env.release() );
	}
	catch( const std::exception & )
	{
		return -1;
	}

	return 0;
}

extern "C" int convert_value(
	converter * handle, 
	const char * source_value,
	int * receiver )
{
	try
	{
		auto env = reinterpret_cast< so_5::wrapped_env_t * >( handle );

		// Do a synchronous request to mbox with fixed name.
		// An exception will be throw in the case of an error.
		*receiver = so_5::request_value< int, std::string >(
			env->environment().create_mbox( "converter" ),
			so_5::infinite_wait,
			source_value );
	}
	catch( const std::exception & )
	{
		return -2;
	}

	return 0;
}

extern "C" void destroy_converter( converter * handle )
{
	auto env = reinterpret_cast< so_5::wrapped_env_t * >( handle );
	delete env;
}

