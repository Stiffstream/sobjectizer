require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.layer.layer_query" )

	cpp_source( "main.cpp" )
}

