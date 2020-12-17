require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.wrapped_env.work_thread_activity_on'

	cpp_source 'main.cpp'
}

