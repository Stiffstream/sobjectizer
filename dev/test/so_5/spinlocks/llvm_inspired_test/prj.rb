require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	target( "_unit.test.spinlocks.llvm_inspired_test" )

	cpp_source( "main.cpp" )
}

