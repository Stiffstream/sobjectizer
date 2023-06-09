require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.disp.nef_thread_pool.unique_thread_id" )

	cpp_source( "main.cpp" )
}

