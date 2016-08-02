require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.event_handler.drop_subscr_in_lambda_event_handler" )

	cpp_source( "main.cpp" )
}

