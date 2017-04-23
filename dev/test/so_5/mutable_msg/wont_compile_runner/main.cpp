#include <test/so_5/wont_compile_helpers.hpp>

int main()
{
	try
	{
		auto name = [](const char * test_name) {
			return std::string( "test/so_5/mutable_msg/wont_compile_cases/" ) +
				test_name + "/prj.rb";
		};

		wont_compile_helpers::process_all({
			name( "mutable_signal_send" )
		,	name( "mutable_signal_subscribe" )
		,	name( "mutable_signal_subscribe_lambda" )
		,	name( "mutable_signal_subscribe_adhoc" )
		,	name( "mutable_signal_subscribe_adhoc_2" )
		,	name( "mutable_msg_as_argument" )
		,	name( "immutable_msg_as_argument" )
		});

		return 0;
	}
	catch(const std::exception & x)
	{
		std::cerr << "Error: " << x.what() << std::endl;
	}

	return 2;
}

