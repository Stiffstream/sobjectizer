require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.timer_thread.negative_args" )

	cpp_source( "main.cpp" )
}

