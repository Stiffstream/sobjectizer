require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.layer.extra_layer_errors" )

	cpp_source( "main.cpp" )
}

