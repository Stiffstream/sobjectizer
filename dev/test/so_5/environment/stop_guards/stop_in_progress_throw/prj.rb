require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.environment.stop_guards.stop_in_progress_throw'

	cpp_source 'main.cpp'
}

