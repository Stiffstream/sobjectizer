require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_test.so_5.mpsc_mbox_stress" )

	cpp_source( "main.cpp" )
}
