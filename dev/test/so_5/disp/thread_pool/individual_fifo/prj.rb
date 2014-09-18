require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.disp.thread_pool.individual_fifo" )

	cpp_source( "main.cpp" )
}

