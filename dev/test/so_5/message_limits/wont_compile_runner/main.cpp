#include <test/so_5/wont_compile_helpers.hpp>

int main()
{
	try
	{
		auto name = [](const char * test_name) {
			return std::string( "test/so_5/message_limits/wont_compile_cases/" ) +
				test_name + "/prj.rb";
		};

		wont_compile_helpers::process_all({
			name( "default_limit_then_transform" )
		});

		return 0;
	}
	catch(const std::exception & x)
	{
		std::cerr << "Error: " << x.what() << std::endl;
	}

	return 2;
}

