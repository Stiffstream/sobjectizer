require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.disp.nef_thread_pool.max_demands_at_once" )

	cpp_source( "main.cpp" )
}

