require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.disp.adv_thread_pool.subscr_in_safe" )

	cpp_source( "main.cpp" )
}

