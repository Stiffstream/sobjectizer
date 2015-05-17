require 'mxx_ru/cpp'
MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.messages.tuple_as_message" )

	cpp_source( "main.cpp" )
}

