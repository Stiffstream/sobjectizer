require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.disp.binder.correct_unbind_after_throw_on_bind" )

	cpp_source( "main.cpp" )
}

