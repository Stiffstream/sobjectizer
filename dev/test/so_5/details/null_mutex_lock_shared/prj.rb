require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.details.null_mutex_lock_shared'

	cpp_source 'main.cpp'
}

