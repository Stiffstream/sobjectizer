#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdlib>

namespace wont_compile_helpers {

void
process_all(std::initializer_list<std::string> && projects)
{
	for(const auto & p : projects)
	{
		std::cout << "***\n"
			"*** TRYING: " << p << "\n***\n"
			"*** NOTE: there should be a compilation failure!\n"
			"***" << std::endl;

		const auto r = std::system( ("ruby " + p).c_str() );
		if( 0 == r )
			throw std::runtime_error("project '" + p + "' compiled cleanly");
	}

	std::cout << "***\n"
		"*** Expected failures for all projects were happen\n"
		"*** TESTS ARE PASSED\n"
		"***" << std::endl;
}

} /* namespace wont_compile_helpers */

