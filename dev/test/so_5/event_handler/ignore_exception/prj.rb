require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.event_handler.ignore_exception" )

	cpp_source( "main.cpp" )
}

