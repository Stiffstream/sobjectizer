require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.spinlocks.llvm_inspired_test" )

	cpp_source( "main.cpp" )
}

