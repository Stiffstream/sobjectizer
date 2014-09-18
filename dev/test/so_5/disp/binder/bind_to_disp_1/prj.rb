require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.disp.binder.bind_to_disp_1" )

	cpp_source( "main.cpp" )
}

